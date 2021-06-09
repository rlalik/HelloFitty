/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FitterFactory.h"

#include <fstream>
#include <getopt.h>
#include <string>
#include <sys/stat.h>

#include <TCanvas.h>
#include <TChain.h>
#include <TDirectory.h>
#include <TError.h>
#include <TGaxis.h>
#include <TH2.h>
#include <TImage.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TMath.h>
#include <TObjString.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TVector.h>

#include <TMatrixDSym.h>
#include <TVirtualFitter.h>

#define PR(x)                                                                                      \
    std::cout << "++DEBUG: " << #x << " = |" << x << "| (" << __FILE__ << ", " << __LINE__ << ")\n";

bool FitterFactory::verbose_flag = true;

void ParamValues::print() { printf("   %g ( %g, %g ), %d, %d\n", val, l, u, mode, has_limits); }

long int HistFitParams::cnt_total = 0;
long int HistFitParams::cnt_owned = 0;

void HistFitParams::printStats(char* infotext)
{
    if (infotext)
        printf(" (%s) FitParams: total=%ld, owned=%ld\n", infotext, cnt_total, cnt_owned);
    else
        printf(" FitParams: total=%ld, owned=%ld\n", cnt_total, cnt_owned);
}

HistFitParams::HistFitParams()
    : allparnum(0), rebin(0), fit_disabled(kFALSE), pars(0), funSig(nullptr), funBkg(nullptr),
      funSum(nullptr), is_owner(true), backup_p(nullptr), backup_e(nullptr)
{
    ++cnt_total;
    ++cnt_owned;
}

HistFitParams::~HistFitParams()
{
    --cnt_total;
    if (is_owner)
    {
        --cnt_owned;
        cleanup();
    }

    delete[] pars;
}

void HistFitParams::cleanup()
{
    if (funSig) delete funSig;
    if (funBkg) delete funBkg;
    if (funSum) delete funSum;

    funSig = nullptr;
    funBkg = nullptr;
    funSum = nullptr;

    drop();
}

void HistFitParams::setOwner(bool owner)
{
    if (is_owner and !owner) { --cnt_owned; }
    else if (!is_owner and owner)
    {
        ++cnt_owned;
    }

    is_owner = owner;
}

void HistFitParams::init(const TString& h, const TString& fsig, const TString& fbg, Int_t rbn,
                         Double_t f_l, Double_t f_u)
{
    setNewName(h);
    rebin = rbn;
    f_sig = fsig;
    f_bkg = fbg;
    TString funcSig = "f_" + histname + "_sig";
    TString funcBkg = "f_" + histname + "_bkg";

    funSig = new TF1(funcSig, fsig, f_l, f_u);
    funBkg = new TF1(funcBkg, fbg, f_l, f_u);

    funSum = new TF1("f_" + histname, fsig + "+" + fbg, f_l, f_u);
    allparnum = funSum->GetNpar();
    pars = new ParamValues[allparnum];
    fun_l = f_l;
    fun_u = f_u;
}

HistFitParams* HistFitParams::clone(const TString& h) const
{
    HistFitParams* hfp = new HistFitParams;
    hfp->init(h, f_sig, f_bkg, rebin, fun_l, fun_u);
    return hfp;
}

void HistFitParams::setNewName(const TString& new_name)
{
    histname = new_name;
    if (new_name[0] == '@')
    {
        fit_disabled = kTRUE;
        histname.Remove(0, 1);
    }
    funname = "f_" + histname;
    if (funSig and funBkg)
    {
        funSig->SetName("f_" + histname + "_sig");
        funBkg->SetName("f_" + histname + "_bkg");

        funSum->SetName("f_" + histname);
    }
}

void HistFitParams::setParam(Int_t par, Double_t val, ParamValues::FitMode mode)
{
    if (!(par < allparnum)) return;
    pars[par].val = val;
    pars[par].mode = mode;
    pars[par].has_limits = false;

    if (mode == ParamValues::FitMode::FIXED)
        funSum->FixParameter(par, val);
    else
        funSum->SetParameter(par, val);
}

void HistFitParams::setParam(Int_t par, Double_t val, Double_t l, Double_t u,
                             ParamValues::FitMode mode)
{
    if (!(par < allparnum)) return;
    pars[par].val = val;
    pars[par].l = l;
    pars[par].u = u;
    pars[par].mode = mode;
    pars[par].has_limits = true;

    if (mode == ParamValues::FitMode::FIXED)
        funSum->FixParameter(par, val);
    else
    {
        funSum->SetParameter(par, val);
        funSum->SetParLimits(par, l, u);
    }
}

HistFitParams* HistFitParams::parseEntryFromFile(const TString& line)
{
    TString line_ = line;
    line_.ReplaceAll("\t", " ");
    TObjArray* arr = line_.Tokenize(" ");

    HistFitParams* hfp = new HistFitParams;

    if (arr->GetEntries() < 5) return hfp;

    hfp->init(((TObjString*)arr->At(0))->String(),        // hist name
              ((TObjString*)arr->At(1))->String(),        // func val
              ((TObjString*)arr->At(2))->String(),        // func val
              ((TObjString*)arr->At(3))->String().Atoi(), // bg par offset
              ((TObjString*)arr->At(4))->String().Atof(), // low range
              ((TObjString*)arr->At(5))->String().Atof()  // upper range
    );

    Double_t par_, l_, u_;
    Int_t step = 0;
    Int_t parnum = 0;
    ParamValues::FitMode flag_;
    bool has_limits_ = false;

    for (int i = 6; i < arr->GetEntries(); /*++i*/ i += step, ++parnum)
    {
        TString val = ((TObjString*)arr->At(i))->String();
        TString nval =
            ((i + 1) < arr->GetEntries()) ? ((TObjString*)arr->At(i + 1))->String() : TString();

        par_ = val.Atof();
        if (nval == ":")
        {
            l_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 2))->String().Atof() : 0;
            u_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 3))->String().Atof() : 0;
            step = 4;
            flag_ = ParamValues::FitMode::FREE;
            has_limits_ = true;
        }
        else if (nval == "F")
        {
            l_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 2))->String().Atof() : 0;
            u_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 3))->String().Atof() : 0;
            step = 4;
            flag_ = ParamValues::FitMode::FIXED;
            has_limits_ = true;
        }
        else if (nval == "f")
        {
            l_ = 0;
            u_ = 0;
            step = 2;
            flag_ = ParamValues::FitMode::FIXED;
            has_limits_ = false;
        }
        else
        {
            l_ = 0;
            u_ = 0;
            step = 1;
            flag_ = ParamValues::FitMode::FREE;
            has_limits_ = false;
        }

        // 		std::cout << parnum << ", " << par_ << ", " << l_ << ", " << u_ << ", " << flag_ <<
        // "\n";
        if (has_limits_)
            hfp->setParam(parnum, par_, l_, u_, flag_);
        else
            hfp->setParam(parnum, par_, flag_);
    }

    return hfp;
}

TString HistFitParams::exportEntry() const
{
    TString out;
    if (fit_disabled)
        out += "@";
    else
        out += " ";

    char sep;

    out = TString::Format("%s%s\t%s %s %d %.0f %.0f", out.Data(), histname.Data(), f_sig.Data(),
                          f_bkg.Data(), rebin, fun_l, fun_u);

    for (int i = 0; i < allparnum; ++i)
    {
        // BUG why val from funSum?
        // 		Double_t val = funSum->GetParameter(i);
        // 		TString v = TString::Format("%g", val);

        TString v = TString::Format("%g", pars[i].val);
        TString l = TString::Format("%g", pars[i].l);
        TString u = TString::Format("%g", pars[i].u);

        switch (pars[i].mode)
        {
            case ParamValues::FitMode::FREE:
                if (pars[i].has_limits)
                    sep = ':';
                else
                    sep = ' ';
                break;
            case ParamValues::FitMode::FIXED:
                if (pars[i].has_limits)
                    sep = 'F';
                else
                    sep = 'f';
                break;
            default:
                sep = ' ';
                break;
        }

        //		pars[i].print();
        if (pars[i].mode == ParamValues::FitMode::FREE and pars[i].has_limits == 0)
            out += TString::Format(" %s", v.Data());
        else if (pars[i].mode == ParamValues::FitMode::FIXED and pars[i].has_limits == 0)
            out += TString::Format(" %s %c", v.Data(), sep);
        else
            out += TString::Format(" %s %c %s %s", v.Data(), sep, l.Data(), u.Data());
    }

    return out;
}

void HistFitParams::print(bool detailed) const
{
    std::cout << "@ hist name = " << histname.Data() << std::endl;
    std::cout << "  func name = " << funname.Data() << std::endl;
    std::cout << " | rebin = " << rebin << std::endl;
    std::cout << "  range = " << fun_l << " -- " << fun_u << std::endl;
    std::cout << "  param num = " << allparnum << std::endl;
    std::cout << "  params list:" << std::endl;

    for (int i = 0; i < allparnum; ++i)
    {
        std::cout << "  * " << i << ": " << pars[i].val << " " << pars[i].l << " " << pars[i].u
                  << std::endl;
    }

    if (detailed)
    {
        std::cout << "+++++++++ SIG function +++++++++" << std::endl;
        if (funSig) funSig->Print("V");
        std::cout << "+++++++++ BKG function +++++++++" << std::endl;
        if (funBkg) funBkg->Print("V");
        std::cout << "+++++++++ SUM function +++++++++" << std::endl;
        if (funSum) funSum->Print("V");
        std::cout << "++++++++++++++++++++++++++++++++" << std::endl;
    }
}

void HistFitParams::printInline() const
{
    std::cout << "  hn=" << histname.Data() << std::endl;
    if (rebin > 0) std::cout << " R=" << rebin;
}

bool HistFitParams::update() { return update(funSum); }

bool HistFitParams::update(TF1* f)
{
    if (allparnum == f->GetNpar())
    {
        for (int i = 0; i < allparnum; ++i)
            pars[i].val = f->GetParameter(i);
    }
    else
        return false;

    return true;
}

void HistFitParams::push()
{
    int n = funSum->GetNpar();
    if (!backup_p)
    {
        backup_p = new double[n];
        backup_e = new double[n];
    }

    for (int i = 0; i < n; ++i)
    {
        backup_p[i] = funSum->GetParameter(i);
        backup_e[i] = funSum->GetParError(i);
    }
}

void HistFitParams::pop()
{
    apply();
    drop();
}

void HistFitParams::apply()
{
    if (!backup_p) return;

    int n = funSum->GetNpar();
    for (int i = 0; i < n; ++i)
    {
        funSum->SetParameter(i, backup_p[i]);
        funSum->SetParError(i, backup_e[i]);
    }
}

void HistFitParams::drop()
{
    if (backup_p)
    {
        delete[] backup_p;
        delete[] backup_e;
    }

    backup_p = backup_e = nullptr;
}

FitterFactory::FitterFactory(Flags flags)
    : flags(flags), has_defaults(false), defpars(nullptr), min_entries(0), ps_prefix(PSFIX::IGNORE),
      ps_suffix(PSFIX::IGNORE)
{
}

FitterFactory::~FitterFactory()
{
    for (auto it = hfpmap.begin(); it != hfpmap.end(); ++it)
        it->second->cleanup();

    hfpmap.clear();
}

FitterFactory::Flags FitterFactory::setFlags(FitterFactory::Flags new_flags)
{
    FitterFactory::Flags old = flags;
    flags = new_flags;
    return old;
}

void FitterFactory::setDefaultParameters(HistFitParams* defs)
{
    has_defaults = true;
    defpars = defs;
}

bool FitterFactory::initFactoryFromFile(const char* filename, const char* auxname)
{
    struct stat st_ref;
    struct stat st_aux;

    TSystem sys;

    par_ref = filename;
    par_aux = auxname;

    if (!filename) { fprintf(stderr, "No reference input file given\n"); }
    if (!auxname) { fprintf(stderr, "No output file given\n"); }

    long long int mod_ref = 0;
    long long int mod_aux = 0;

    //	#ifdef HAVE_ST_MTIM
    if (stat(filename, &st_ref)) { perror(filename); }
    else
    {
        mod_ref = (long long)st_ref.st_mtim.tv_sec;
    }

    if (stat(auxname, &st_aux)) { perror(auxname); }
    else
    {
        mod_aux = (long long)st_aux.st_mtim.tv_sec;
    }

    bool aux_newer = mod_aux > mod_ref;

    std::cout << "Parameter files:";
    if (!filename)
        std::cout << " [x] REF";
    else if (!aux_newer)
        std::cout << " [*] REF";
    else
        std::cout << " [ ] REF";

    if (!auxname)
        std::cout << " [x] AUX";
    else if (aux_newer)
        std::cout << " [*] AUX";
    else
        std::cout << " [ ] AUX";
    std::cout << std::endl;

    if (flags == Flags::ALWAYS_REF) return import_parameters(filename);

    if (flags == Flags::ALWAYS_AUX) return import_parameters(auxname);

    if (flags == Flags::ALWAYS_NEWER)
    {
        if (aux_newer)
            return import_parameters(auxname);
        else
            return import_parameters(filename);
    }

    return false;
}

bool FitterFactory::exportFactoryToFile() { return export_parameters(par_aux); }

void FitterFactory::insertParameters(HistFitParams* hfp)
{
    HfpEntry par(hfp->histname, hfp);
    hfpmap.insert(par);
}

void FitterFactory::insertParameters(const TString& name, HistFitParams* hfp)
{
    HfpEntry par(name, hfp);
    hfpmap.insert(par);
}

void FitterFactory::insertParameters(const HfpEntry& par) { hfpmap.insert(par); }

bool FitterFactory::import_parameters(const std::string& filename)
{
    std::ifstream ifs(filename.c_str());
    if (!ifs.is_open())
    {
        std::cerr << "No file " << filename << " to open." << std::endl;
        return false;
    }

    size_t cnt = 0;
    std::string line;
    while (std::getline(ifs, line))
    {
        HistFitParams* imfit = HistFitParams::parseEntryFromFile(line);
        insertParameters(imfit);
        ++cnt;
    }

    printf("Imported %lu entries, total entries %lu\n", cnt, hfpmap.size());
    return true;
}

bool FitterFactory::export_parameters(const std::string& filename)
{
    if (filename.empty()) return false;

    std::ofstream fparfile(filename);
    if (!fparfile.is_open())
    {
        std::cerr << "Can't create AUX file " << filename << ". Skipping..." << std::endl;
    }
    else
    {
        std::cout << "AUX file " << filename << " opened..." << std::endl;
        for (auto it = hfpmap.begin(); it != hfpmap.end(); ++it)
        {
            fparfile << it->second->exportEntry().Data() << std::endl;
        }
    }
    fparfile.close();
    return true;
}

// bool FitterFactory::updateParams(TH1 * hist, HistFitParams & hfp)
// {
// 	auto it = hfpmap.find(format_name(hist->GetName()));
// 	if (it != hfpmap.end())
// 	{
// 		it->second = hfp;
// 	}
// 	else
// 		return false;
//
// 	return true;
// }
//
// bool FitterFactory::updateParams(TH1 * hist, TF1 * f)
// {
// 	auto it = hfpmap.find(format_name(hist->GetName()));
// 	if (it != hfpmap.end())
// 	{
// 		HistFitParams * hfp = it->second;
//
// 		return hfp->update(f);
// 	}
// 	else
// 		return false;
//
// 	return true;
// }

HistFitParams* FitterFactory::findParams(TH1* hist) const { return findParams(hist->GetName()); }

HistFitParams* FitterFactory::findParams(const char* name) const
{
    auto it = hfpmap.find(format_name(name));
    if (it != hfpmap.end()) return it->second;

    return nullptr;
}

bool FitterFactory::fit(TH1* hist, const char* pars, const char* gpars)
{
    HistFitParams* hfp = findParams(hist->GetName());
    if (!hfp)
    {
        hfp = defpars->clone(format_name(hist->GetName()));
        insertParameters(hfp);
    }

    hfp->push();
    bool status = fit(hfp, hist, pars, gpars, min_entries);

    if (!status) hfp->pop();

    return status;
}

bool FitterFactory::fit(HistFitParams* hfp, TH1* hist, const char* pars, const char* gpars,
                        double min_entries)
{
    Int_t bin_l = hist->FindBin(hfp->fun_l);
    Int_t bin_u = hist->FindBin(hfp->fun_u);

    if (hfp->rebin != 0)
    {
        // 		was_rebin = true;
        hist->Rebin(hfp->rebin);
    }

    if (hist->GetEntries() < min_entries) return false;

    if (hist->Integral(bin_l, bin_u) == 0) return false;

    TF1* tfSig = hfp->funSig;
    TF1* tfBkg = hfp->funBkg;
    TF1* tfSum = hfp->funSum;

    hist->GetListOfFunctions()->Clear();
    hist->GetListOfFunctions()->SetOwner(kTRUE);

    tfSig->SetBit(TF1::kNotDraw);
    tfSig->SetBit(TF1::kNotGlobal);
    tfBkg->SetBit(TF1::kNotDraw);
    tfBkg->SetBit(TF1::kNotGlobal);
    tfSum->SetBit(TF1::kNotDraw);
    tfSum->SetBit(TF1::kNotGlobal);

    const size_t par_num = tfSum->GetNpar();

    // backup old parameters
    double* pars_backup_old = new double[par_num];
    tfSum->GetParameters(pars_backup_old);
    double chi2_backup_old = hist->Chisquare(tfSum, "R");

    if (verbose_flag)
    {
        // print them
        printf("* old: ");
        for (uint i = 0; i < par_num; ++i)
            printf("%g ", pars_backup_old[i]);
        printf(" --> chi2:  %f -- *\n", chi2_backup_old);
    }

    hist->Fit(tfSum, pars, gpars, hfp->fun_l, hfp->fun_u);

    TF1* new_sig_func = ((TF1*)hist->GetListOfFunctions()->At(0));

    // 	TVirtualFitter * fitter = TVirtualFitter::GetFitter();
    // 	TMatrixDSym cov;
    // 	fitter->GetCovarianceMatrix()
    // 	cov.Use(fitter->GetNumberTotalParameters(), fitter->GetCovarianceMatrix());
    // 	cov.Print();

    // backup new parameters
    double* pars_backup_new = new double[par_num];
    tfSum->GetParameters(pars_backup_new);
    double chi2_backup_new = hist->Chisquare(tfSum, "R");

    if (verbose_flag)
    {
        printf("  new: ");
        for (uint i = 0; i < par_num; ++i)
            printf("%g ", pars_backup_new[i]);
        printf(" --> chi2:  %f -- *", chi2_backup_new);
    }

    if (chi2_backup_new > chi2_backup_old)
    {
        tfSum->SetParameters(pars_backup_old);
        new_sig_func->SetParameters(pars_backup_old);
        printf("\n\tFIT-ERROR: Fit got worse -> restoring params for chi2 = %g",
               hist->Chisquare(tfSum, "R"));
    }
    else if (tfSum->GetMaximum() > 2.0 * hist->GetMaximum())
    {
        tfSum->SetParameters(pars_backup_old);
        new_sig_func->SetParameters(pars_backup_old);
        printf("\n\tMAX-ERROR: %g vs. %g -> %f (entries=%g)", tfSum->GetMaximum(),
               hist->GetMaximum(), hist->Chisquare(tfSum, "R"), hist->GetEntries());
        printf("\n");

        printf("  old: ");
        for (uint i = 0; i < par_num; ++i)
            printf("%g ", pars_backup_old[i]);
        printf(" --> chi2:  %f -- *\n", chi2_backup_old);

        printf("  new: ");
        for (uint i = 0; i < par_num; ++i)
            printf("%g ", pars_backup_new[i]);
        printf(" --> chi2:  %f -- *\n", chi2_backup_new);

        for (int i = 0; i < hist->GetNbinsX(); ++i)
            printf(" %g", hist->GetBinContent(i + 1));
        printf("\n");
    }
    else
    {
        // 		printf("\n\tIS-OK: %g vs. %g -> %f", tfSum->GetMaximum(), hist->GetMaximum(),
        // 			   hist->Chisquare(tfSum, "R") );

        if (verbose_flag) printf("\t [ OK ]\n");
    }

    tfSum->SetChisquare(hist->Chisquare(tfSum, "R"));

    new_sig_func->SetChisquare(hist->Chisquare(tfSum, "R"));

    uint parnsig = tfSig->GetNpar();
    for (int i = 0; i < tfSig->GetNpar(); ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        tfSig->SetParameter(i, par);
        tfSig->SetParError(i, err);
    }

    for (int i = parnsig; i < tfBkg->GetNpar(); ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        tfBkg->SetParameter(i, par);
        tfBkg->SetParError(i, err);
    }

    //     hist->GetListOfFunctions()->Add(tfSum);
    hist->GetListOfFunctions()->Add(tfSig);
    hist->GetListOfFunctions()->Add(tfBkg);

    delete[] pars_backup_old;
    delete[] pars_backup_new;

    return true;
}

void FitterFactory::print() const
{
    for (auto it = hfpmap.begin(); it != hfpmap.end(); ++it)
        it->second->print();
}

void FitterFactory::cleanup()
{
    for (auto it = hfpmap.begin(); it != hfpmap.end(); ++it)
        it->second->cleanup();

    hfpmap.clear();
}

std::string FitterFactory::format_name(const std::string& name) const
{
    if (ps_prefix == PSFIX::IGNORE and ps_suffix == PSFIX::IGNORE and !rep_src.length())
        return name;

    std::string formatted = name;

    if (ps_prefix == PSFIX::APPEND and ps_suffix == PSFIX::APPEND) return prefix + name + suffix;

    if (ps_prefix == PSFIX::APPEND) formatted = prefix + formatted;

    if (ps_suffix == PSFIX::APPEND) formatted = formatted + suffix;

    if (ps_prefix == PSFIX::SUBSTRACT)
    {
        size_t pos = formatted.find(prefix);
        if (pos == 0) formatted = formatted.substr(prefix.length(), std::string::npos);
    }

    if (ps_suffix == PSFIX::SUBSTRACT)
    {
        size_t pos = formatted.find(suffix, formatted.length() - suffix.length());
        if (pos != std::string::npos) formatted = formatted.substr(0, pos);
    }

    if (rep_src.length())
    {
        size_t pos = formatted.find(rep_src);
        if (pos != std::string::npos) formatted = formatted.replace(pos, rep_src.length(), rep_dst);
    }

    return formatted;
}

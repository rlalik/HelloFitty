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

#include <TF1.h>
#include <TH1.h>
#include <TList.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <TSystem.h>

#include <fstream>
#include <string>
#include <sys/stat.h>

#define PR(x)                                                                                      \
    std::cout << "++DEBUG: " << #x << " = |" << x << "| (" << __FILE__ << ", " << __LINE__ << ")\n";

bool FitterFactory::verbose_flag = true;

void ParamValue::print()
{
    printf("   %g ( %g, %g ), %s, %d\n", val, l, u, mode == FitMode::Free ? "Free" : "Fixed",
           has_limits);
}

HistogramFitParams::HistogramFitParams(const TString& hist_name, const TString& formula_s,
                                       const TString& formula_b, Double_t range_lower,
                                       Double_t range_upper)
    : hist_name(hist_name), sig_string(formula_s), bkg_string(formula_b), range_l(range_lower),
      range_u(range_upper),
      function_sig("", formula_s, range_lower, range_upper, TF1::EAddToList::kNo),
      function_bkg("", formula_b, range_lower, range_upper, TF1::EAddToList::kNo),
      function_sum("", formula_s + "+" + formula_b, range_lower, range_upper, TF1::EAddToList::kNo)
{
    pars.resize(function_sum.GetNpar());
}

auto HistogramFitParams::clone(const TString& new_name) const -> std::unique_ptr<HistogramFitParams>
{
    return std::make_unique<HistogramFitParams>(new_name, sig_string, bkg_string, range_l, range_u);
}

void HistogramFitParams::clear() { drop(); }

void HistogramFitParams::init(const TString& function_format_name)
{
    if (hist_name[0] == '@')
    {
        fit_disabled = true;
        return;
    }

    for (auto i = 0; i < pars.size(); ++i)
        if (pars[i].mode == ParamValue::FitMode::Fixed)
            function_sum.FixParameter(i, pars[i].val);
        else
            function_sum.SetParameter(i, pars[i].val);
}

void HistogramFitParams::setParam(Int_t par, ParamValue value)
{
    if (!(par < pars.size())) return;
    pars[par] = value;
}

void HistogramFitParams::setParam(Int_t par, Double_t val, ParamValue::FitMode mode)
{
    if (!(par < pars.size())) return;
    pars[par].val = val;
    pars[par].l = 0;
    pars[par].u = 0;
    pars[par].mode = mode;
    pars[par].has_limits = false;
}

void HistogramFitParams::setParam(Int_t par, Double_t val, Double_t l, Double_t u,
                                  ParamValue::FitMode mode)
{
    if (!(par < pars.size())) return;
    pars[par].val = val;
    pars[par].l = l;
    pars[par].u = u;
    pars[par].mode = mode;
    pars[par].has_limits = true;
}

std::unique_ptr<HistogramFitParams> HistogramFitParams::parseEntryFromFile(const TString& line)
{
    TString line_ = line;
    line_.ReplaceAll("\t", " ");
    TObjArray* arr = line_.Tokenize(" ");

    if (arr->GetEntries() < 5)
    {
        std::cerr << "Error parsing line:\n " << line << "\n";
        abort();
    };

    auto hfp = std::make_unique<HistogramFitParams>(
        ((TObjString*)arr->At(0))->String(),        // hist name
        ((TObjString*)arr->At(1))->String(),        // func val
        ((TObjString*)arr->At(2))->String(),        // func val
        ((TObjString*)arr->At(4))->String().Atof(), // low range
        ((TObjString*)arr->At(5))->String().Atof());

    Double_t par_, l_, u_;
    Int_t step = 0;
    Int_t parnum = 0;
    ParamValue::FitMode flag_;
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
            flag_ = ParamValue::FitMode::Free;
            has_limits_ = true;
        }
        else if (nval == "F")
        {
            l_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 2))->String().Atof() : 0;
            u_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 3))->String().Atof() : 0;
            step = 4;
            flag_ = ParamValue::FitMode::Fixed;
            has_limits_ = true;
        }
        else if (nval == "f")
        {
            l_ = 0;
            u_ = 0;
            step = 2;
            flag_ = ParamValue::FitMode::Fixed;
            has_limits_ = false;
        }
        else
        {
            l_ = 0;
            u_ = 0;
            step = 1;
            flag_ = ParamValue::FitMode::Free;
            has_limits_ = false;
        }

        if (has_limits_)
            hfp->setParam(parnum, par_, l_, u_, flag_);
        else
            hfp->setParam(parnum, par_, flag_);
    }

    return hfp;
}

TString HistogramFitParams::exportEntry() const
{
    TString out;
    if (fit_disabled)
        out += "@";
    else
        out += " ";

    char sep;

    out = TString::Format("%s%s\t%s %s %d %.0f %.0f", out.Data(), hist_name.Data(),
                          sig_string.Data(), bkg_string.Data(), rebin, range_l, range_u);
    auto limit = pars.size();

    for (decltype(limit) i = 0; i < limit; ++i)
    {
        TString v = TString::Format("%g", pars[i].val);
        TString l = TString::Format("%g", pars[i].l);
        TString u = TString::Format("%g", pars[i].u);

        switch (pars[i].mode)
        {
            case ParamValue::FitMode::Free:
                if (pars[i].has_limits)
                    sep = ':';
                else
                    sep = ' ';
                break;
            case ParamValue::FitMode::Fixed:
                if (pars[i].has_limits)
                    sep = 'F';
                else
                    sep = 'f';
                break;
            default:
                sep = ' ';
                break;
        }

        if (pars[i].mode == ParamValue::FitMode::Free and pars[i].has_limits == 0)
            out += TString::Format(" %s", v.Data());
        else if (pars[i].mode == ParamValue::FitMode::Fixed and pars[i].has_limits == 0)
            out += TString::Format(" %s %c", v.Data(), sep);
        else
            out += TString::Format(" %s %c %s %s", v.Data(), sep, l.Data(), u.Data());
    }

    return out;
}

void HistogramFitParams::print(bool detailed) const
{
    std::cout << "@ hist name = " << hist_name.Data() << std::endl;
    std::cout << "  func name = " << hist_name.Data() << std::endl;
    std::cout << " | rebin = " << rebin << std::endl;
    std::cout << "  range = " << range_l << " -- " << range_u << std::endl;
    std::cout << "  param num = " << pars.size() << std::endl;
    std::cout << "  params list:" << std::endl;

    auto s = pars.size();
    for (decltype(s) i = 0; i < s; ++i)
    {
        std::cout << "  * " << i << ": " << pars[i].val << " " << pars[i].l << " " << pars[i].u
                  << std::endl;
    }

    if (detailed)
    {
        std::cout << "+++++++++ SIG function +++++++++" << std::endl;
        function_sig.Print("V");
        std::cout << "+++++++++ BKG function +++++++++" << std::endl;
        function_bkg.Print("V");
        std::cout << "+++++++++ SUM function +++++++++" << std::endl;
        function_sum.Print("V");
        std::cout << "++++++++++++++++++++++++++++++++" << std::endl;
    }
}

void HistogramFitParams::printInline() const
{
    std::cout << "  hn=" << hist_name.Data() << std::endl;
    if (rebin > 0) std::cout << " R=" << rebin;
}

bool HistogramFitParams::load(TF1* f)
{
    auto s = pars.size();
    if (s == f->GetNpar())
    {
        for (int i = 0; i < s; ++i)
            pars[i].val = f->GetParameter(i);
    }
    else
        return false;

    return true;
}

void HistogramFitParams::push()
{
    int n = function_sum.GetNpar();
    if (backup_p.size() == 0)
    {
        backup_p.resize(n);
        backup_e.resize(n);

        for (int i = 0; i < n; ++i)
        {
            backup_p[i] = function_sum.GetParameter(i);
            backup_e[i] = function_sum.GetParError(i);
        }
    }
}

void HistogramFitParams::pop()
{
    apply();
    drop();
}

void HistogramFitParams::apply()
{
    auto s = pars.size();
    auto n = function_sum.GetNpar();

    if (s != n) return;

    for (int i = 0; i < n; ++i)
    {
        function_sum.SetParameter(i, backup_p[i]);
        function_sum.SetParError(i, backup_e[i]);
    }
}

void HistogramFitParams::drop()
{
    backup_p.clear();
    backup_e.clear();
}

FitterFactory::FitterFactory(PriorityMode mode)
    : mode(mode), has_defaults(false), defpars(nullptr), min_entries(0)
{
}

FitterFactory::~FitterFactory() { clear(); }

FitterFactory::PriorityMode FitterFactory::setFlags(FitterFactory::PriorityMode new_mode)
{
    FitterFactory::PriorityMode old = mode;
    mode = new_mode;
    return old;
}

void FitterFactory::setDefaultParameters(HistogramFitParams* defs)
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

    if (mode == PriorityMode::Reference) return import_parameters(filename);

    if (mode == PriorityMode::Auxilary) return import_parameters(auxname);

    if (mode == PriorityMode::Newer)
    {
        if (aux_newer)
            return import_parameters(auxname);
        else
            return import_parameters(filename);
    }

    return false;
}

bool FitterFactory::exportFactoryToFile() { return export_parameters(par_aux); }

void FitterFactory::insertParameters(std::unique_ptr<HistogramFitParams> hfp)
{
    auto n = hfp->hist_name;
    insertParameters(n, std::move(hfp));
}

void FitterFactory::insertParameters(const TString& name, std::unique_ptr<HistogramFitParams> hfp)
{
    hfp->init(format_name(name, name_decorator));
    hfpmap.insert({name, std::move(hfp)});
}

bool FitterFactory::import_parameters(std::string_view filename)
{
    std::string fn(filename);
    std::ifstream fparfile(fn);
    if (!fparfile.is_open())
    {
        std::cerr << "No file " << filename << " to open." << std::endl;
        return false;
    }

    size_t cnt = 0;
    std::string line;
    while (std::getline(fparfile, line))
    {
        insertParameters(HistogramFitParams::parseEntryFromFile(line));
        ++cnt;
    }

    return true;
}

bool FitterFactory::export_parameters(std::string_view filename)
{
    std::string fn(filename);
    std::ofstream fparfile(fn);
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

HistogramFitParams* FitterFactory::findParams(TH1* hist) const
{
    return findParams(hist->GetName());
}

HistogramFitParams* FitterFactory::findParams(const char* name) const
{
    auto it = hfpmap.find(format_name(name, name_decorator));
    if (it != hfpmap.end()) return it->second.get();

    return nullptr;
}

bool FitterFactory::fit(TH1* hist, const char* pars, const char* gpars)
{
    HistogramFitParams* hfp = findParams(hist->GetName());
    if (!hfp)
    {
        printf("HFP for histogram %s not found, trying from defaults.\n", hist->GetName());

        if (!defpars) return false;

        auto tmp = defpars->clone(format_name(hist->GetName(), name_decorator));
        hfp = tmp.get();
        insertParameters(std::move(tmp));
    }

    hfp->push();
    bool status = fit(hfp, hist, pars, gpars, min_entries);

    if (!status) hfp->pop();

    return status;
}

bool FitterFactory::fit(HistogramFitParams* hfp, TH1* hist, const char* pars, const char* gpars,
                        double min_entries)
{
    Int_t bin_l = hist->FindBin(hfp->range_l);
    Int_t bin_u = hist->FindBin(hfp->range_u);

    if (hfp->rebin != 0)
    {
        // was_rebin = true;
        hist->Rebin(hfp->rebin);
    }

    if (hist->GetEntries() < min_entries) return false;

    if (hist->Integral(bin_l, bin_u) == 0) return false;

    TF1* tfSig =
        (TF1*)hfp->function_sig.Clone(format_name(hfp->hist_name, function_decorator + "_sig"));
    TF1* tfBkg =
        (TF1*)hfp->function_bkg.Clone(format_name(hfp->hist_name, function_decorator + "_bkg"));
    TF1* tfSum = (TF1*)hfp->function_sum.Clone(format_name(hfp->hist_name, function_decorator));

    hist->GetListOfFunctions()->Clear();
    hist->GetListOfFunctions()->SetOwner(kTRUE); // FIXME do we ened this?

    tfSig->SetBit(TF1::kNotDraw);
    tfSig->SetBit(TF1::kNotGlobal);
    tfBkg->SetBit(TF1::kNotDraw);
    tfBkg->SetBit(TF1::kNotGlobal);
    // tfSum->SetBit(TF1::kNotDraw);
    // tfSum->SetBit(TF1::kNotGlobal);

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

    hist->Fit(tfSum, pars, gpars, hfp->range_l, hfp->range_u);

    TF1* new_sig_func = ((TF1*)hist->GetListOfFunctions()->At(0));

    // TVirtualFitter * fitter = TVirtualFitter::GetFitter();
    // TMatrixDSym cov;
    // fitter->GetCovarianceMatrix()
    // cov.Use(fitter->GetNumberTotalParameters(), fitter->GetCovarianceMatrix());
    // cov.Print();

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
        // printf("\n\tIS-OK: %g vs. %g -> %f", tfSum->GetMaximum(), hist->GetMaximum(),
        //        hist->Chisquare(tfSum, "R"));

        if (verbose_flag) printf("\t [ OK ]\n");
    }

    tfSum->SetChisquare(hist->Chisquare(tfSum, "R"));

    new_sig_func->SetChisquare(hist->Chisquare(tfSum, "R"));

    uint parnsig = tfSig->GetNpar();
    for (int i = 0; i < parnsig; ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        tfSig->SetParameter(i, par);
        tfSig->SetParError(i, err);

        hfp->pars[i].val = par;
    }

    for (int i = parnsig; i < tfBkg->GetNpar(); ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        tfBkg->SetParameter(i, par);
        tfBkg->SetParError(i, err);

        hfp->pars[i].val = par;
    }

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

void FitterFactory::clear() { hfpmap.clear(); }

TString FitterFactory::format_name(const TString& name, const TString& decorator) const
{
    TString s = decorator;
    s.ReplaceAll("*", name);
    return s;
}

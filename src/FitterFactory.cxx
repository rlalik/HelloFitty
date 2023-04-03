/*
    FitterFactory - mass fitting tool for CERN's ROOT applications
    Copyright (C) 2015-2021  Rafał Lalik <rafallalik@gmail.com>

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

#include <fstream>
#include <string>

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <sys/stat.h>
#endif

#define PR(x)                                                                                      \
    std::cout << "++DEBUG: " << #x << " = |" << x << "| (" << __FILE__ << ", " << __LINE__ << ")\n";

#if __cplusplus < 201402L
template <typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
using std::make_unique;
#endif

struct FitterFactoryImpl
{
    FitterFactory::PriorityMode mode;

    static bool verbose_flag;

    TString par_ref;
    TString par_aux;

    HistogramFit* defpars{nullptr};
    std::map<TString, std::unique_ptr<HistogramFit>> hfpmap;

    TString name_decorator{"*"};
    TString function_decorator{"f_*"};

    TString rep_src;
    TString rep_dst;

    bool draw_sum{true}, draw_sig{false}, draw_bkg{false};
    FitterFactoryTools::DrawProperties prop_sum, prop_sig, prop_bkg;
};

bool FitterFactoryImpl::verbose_flag = true;

void ParamValue::print() const
{
    printf("%10g   Mode: %-5s   Limits: ", val, mode == FitMode::Free ? "Free" : "Fixed");
    if (has_limits)
        printf(" %g, %g\n", l, u);
    else
        printf(" none\n");
}

HistogramFit::HistogramFit(const TString& hist_name, const TString& formula_s,
                           const TString& formula_b, Double_t range_lower, Double_t range_upper)
    : hist_name(hist_name), sig_string(formula_s), bkg_string(formula_b), range_l(range_lower),
      range_u(range_upper),
      function_sig("", formula_s, range_lower, range_upper, TF1::EAddToList::kNo),
      function_bkg("", formula_b, range_lower, range_upper, TF1::EAddToList::kNo),
      function_sum("", formula_s + "+" + formula_b, range_lower, range_upper, TF1::EAddToList::kNo)
{
    if (hist_name[0] == '@') { fit_disabled = true; }

    pars.resize(function_sum.GetNpar());
}

auto HistogramFit::clone(const TString& new_name) const -> std::unique_ptr<HistogramFit>
{
    return make_unique<HistogramFit>(new_name, sig_string, bkg_string, range_l, range_u);
}

void HistogramFit::clear() { drop(); }

void HistogramFit::init()
{
    for (auto i = 0; i < pars.size(); ++i)
        if (pars[i].mode == ParamValue::FitMode::Fixed)
            function_sum.FixParameter(i, pars[i].val);
        else
        {
            function_sum.SetParameter(i, pars[i].val);
            if (pars[i].has_limits) function_sum.SetParLimits(i, pars[i].l, pars[i].u);
        }
}

void HistogramFit::setParam(Int_t par, ParamValue value)
{
    if (!(par < pars.size())) return;
    pars[par] = value;
}

void HistogramFit::setParam(Int_t par, Double_t val, ParamValue::FitMode mode)
{
    if (!(par < pars.size())) return;
    pars[par].val = val;
    pars[par].l = 0;
    pars[par].u = 0;
    pars[par].mode = mode;
    pars[par].has_limits = false;
}

void HistogramFit::setParam(Int_t par, Double_t val, Double_t l, Double_t u,
                            ParamValue::FitMode mode)
{
    if (!(par < pars.size())) return;
    pars[par].val = val;
    pars[par].l = l;
    pars[par].u = u;
    pars[par].mode = mode;
    pars[par].has_limits = true;
}

std::unique_ptr<HistogramFit> HistogramFit::parseLineEntry(const TString& line)
{
    TString line_ = line;
    line_.ReplaceAll("\t", " ");
    TObjArray* arr = line_.Tokenize(" ");

    if (arr->GetEntries() < 6)
    {
        std::cerr << "Error parsing line:\n " << line << "\n";
        delete arr;
        return nullptr;
    };

    auto hfp = make_unique<HistogramFit>(((TObjString*)arr->At(0))->String(),        // hist name
                                         ((TObjString*)arr->At(1))->String(),        // func val
                                         ((TObjString*)arr->At(2))->String(),        // func val
                                         ((TObjString*)arr->At(4))->String().Atof(), // low range
                                         ((TObjString*)arr->At(5))->String().Atof());

    auto npars = hfp->pars.size();

    Double_t par_, l_, u_;
    Int_t step = 0;
    Int_t parnum = 0;
    ParamValue::FitMode flag_;
    bool has_limits_ = false;

    auto entries = arr->GetEntries();
    for (int i = 6; i < entries; i += step, ++parnum)
    {
        if (parnum >= npars)
        {
            delete arr;
            return nullptr;
        }

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

    delete arr;

    return hfp;
}

TString HistogramFit::exportEntry() const
{
    TString out = fit_disabled ? "@" : " ";

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

void HistogramFit::print(bool detailed) const
{
    std::cout << "## name: " << hist_name.Data() << "   rebin: " << rebin << "   range: " << range_l
              << " -- " << range_u << "  param num: " << pars.size() << "\n";

    auto s = pars.size();
    for (decltype(s) i = 0; i < s; ++i)
    {
        std::cout << "   " << i << ": ";
        pars[i].print();
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

bool HistogramFit::load(TF1* f)
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

void HistogramFit::push()
{
    backup_p.clear();
    for (auto& p : pars)
        backup_p.push_back(p.val);
}

void HistogramFit::pop()
{
    apply();
    drop();
}

void HistogramFit::apply()
{
    if (backup_p.size() != pars.size()) return;

    auto n = pars.size();
    for (decltype(n) i = 0; i < n; ++i)
        pars[i].val = backup_p[i];
}

void HistogramFit::drop() { backup_p.clear(); }

void FitterFactory::setVerbose(bool verbose) { FitterFactoryImpl::verbose_flag = verbose; }

FitterFactory::FitterFactory(PriorityMode mode) : d{std::make_unique<FitterFactoryImpl>()}
{
    d->mode = mode;
    d->defpars = nullptr;
}

FitterFactory::~FitterFactory() { clear(); }

bool FitterFactory::initFactoryFromFile(const char* filename, const char* auxname)
{
    d->par_ref = filename;
    d->par_aux = auxname;

    if (!filename) { fprintf(stderr, "No reference input file given\n"); }
    if (!auxname) { fprintf(stderr, "No output file given\n"); }

    auto selected = FitterFactoryTools::selectSource(filename, auxname);

    if (selected == FitterFactoryTools::SelectedSource::None) return false;

    printf("Available source: [%c] REF  [%c] AUX\n",
           selected != FitterFactoryTools::SelectedSource::OnlyAuxilary and
                   selected != FitterFactoryTools::SelectedSource::None
               ? 'x'
               : ' ',
           selected != FitterFactoryTools::SelectedSource::OnlyReference and
                   selected != FitterFactoryTools::SelectedSource::None
               ? 'x'
               : ' ');
    printf("Selected source : [%c] REF  [%c] AUX\n",
           selected == FitterFactoryTools::SelectedSource::Reference ? 'x' : ' ',
           selected == FitterFactoryTools::SelectedSource::Auxilary ? 'x' : ' ');

    auto mode = d->mode;
    if (mode == PriorityMode::Reference)
    {
        if (selected == FitterFactoryTools::SelectedSource::OnlyAuxilary)
            return false;
        else
            return importParameters(filename);
    }

    if (mode == PriorityMode::Auxilary)
    {
        if (selected == FitterFactoryTools::SelectedSource::OnlyReference)
            return false;
        else
            return importParameters(auxname);
    }

    if (mode == PriorityMode::Newer)
    {
        if (selected == FitterFactoryTools::SelectedSource::Auxilary or
            selected == FitterFactoryTools::SelectedSource::OnlyAuxilary)
            return importParameters(auxname);
        else if (selected == FitterFactoryTools::SelectedSource::Reference or
                 selected == FitterFactoryTools::SelectedSource::OnlyReference)
            return importParameters(filename);
    }

    return false;
}

bool FitterFactory::exportFactoryToFile() { return exportParameters(d->par_aux.Data()); }

void FitterFactory::insertParameters(std::unique_ptr<HistogramFit>&& hfp)
{
    auto n = hfp->hist_name;
    insertParameters(n, std::move(hfp));
}

void FitterFactory::insertParameters(const TString& name, std::unique_ptr<HistogramFit>&& hfp)
{
    d->hfpmap.insert({name, std::move(hfp)});
}

bool FitterFactory::importParameters(const std::string& filename)
{
    std::ifstream fparfile(filename);
    if (!fparfile.is_open())
    {
        std::cerr << "No file " << filename << " to open." << std::endl;
        return false;
    }

    d->hfpmap.clear();

    std::string line;
    while (std::getline(fparfile, line))
    {
        insertParameters(HistogramFit::parseLineEntry(line));
    }

    return true;
}

bool FitterFactory::exportParameters(const std::string& filename)
{
    std::ofstream fparfile(filename);
    if (!fparfile.is_open())
    {
        std::cerr << "Can't create AUX file " << filename << ". Skipping..." << std::endl;
    }
    else
    {
        std::cout << "AUX file " << filename << " opened...  Exporting " << d->hfpmap.size()
                  << " entries.\n";
        for (auto it = d->hfpmap.begin(); it != d->hfpmap.end(); ++it)
        {
            fparfile << it->second->exportEntry().Data() << std::endl;
        }
    }
    fparfile.close();
    return true;
}

HistogramFit* FitterFactory::findFit(TH1* hist) const { return findFit(hist->GetName()); }

HistogramFit* FitterFactory::findFit(const char* name) const
{
    auto it = d->hfpmap.find(format_name(name, d->name_decorator));
    if (it != d->hfpmap.end()) return it->second.get();

    return nullptr;
}

bool FitterFactory::fit(TH1* hist, const char* pars, const char* gpars)
{
    HistogramFit* hfp = findFit(hist->GetName());
    if (!hfp)
    {
        printf("HFP for histogram %s not found, trying from defaults.\n", hist->GetName());

        if (!d->defpars) return false;

        auto tmp = d->defpars->clone(format_name(hist->GetName(), d->name_decorator));
        hfp = tmp.get();
        insertParameters(std::move(tmp));
    }

    hfp->push();
    bool status = fit(hfp, hist, pars, gpars);

    if (!status) hfp->pop();

    return status;
}

bool FitterFactory::fit(HistogramFit* hfp, TH1* hist, const char* pars, const char* gpars)
{
    Int_t bin_l = hist->FindBin(hfp->range_l);
    Int_t bin_u = hist->FindBin(hfp->range_u);

    hfp->init();

    if (hfp->rebin != 0)
    {
        // was_rebin = true;
        hist->Rebin(hfp->rebin);
    }

    if (hist->Integral(bin_l, bin_u) == 0) return false;

    TF1* tfSig = &hfp->function_sig;
    TF1* tfBkg = &hfp->function_bkg;
    TF1* tfSum = &hfp->function_sum;

    tfSig->SetName(format_name(hfp->hist_name, d->function_decorator + "_sig"));
    tfBkg->SetName(format_name(hfp->hist_name, d->function_decorator + "_bkg"));
    tfSum->SetName(format_name(hfp->hist_name, d->function_decorator));

    hist->GetListOfFunctions()->Clear();
    hist->GetListOfFunctions()->SetOwner(kTRUE);

    if (d->draw_sig)
        propSig().applyStyle(tfSig);
    else
        tfSig->SetBit(TF1::kNotDraw);
    // tfSig->SetBit(TF1::kNotGlobal);
    if (d->draw_bkg)
        propBkg().applyStyle(tfBkg);
    else
        tfBkg->SetBit(TF1::kNotDraw);
    // tfBkg->SetBit(TF1::kNotGlobal);
    if (d->draw_sum)
        propSum().applyStyle(tfSum);
    else
        tfSum->SetBit(TF1::kNotDraw);
    // tfSum->SetBit(TF1::kNotGlobal);

    const size_t par_num = tfSum->GetNpar();

    // backup old parameters
    double* pars_backup_old = new double[par_num];
    tfSum->GetParameters(pars_backup_old);
    double chi2_backup_old = hist->Chisquare(tfSum, "R");

    if (d->verbose_flag)
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

    if (d->verbose_flag)
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

        if (d->verbose_flag) printf("\t [ OK ]\n");
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

    hist->GetListOfFunctions()->Add(
        tfSig->Clone(format_name(hfp->hist_name, d->function_decorator + "_sig")));
    hist->GetListOfFunctions()->Add(
        tfBkg->Clone(format_name(hfp->hist_name, d->function_decorator + "_bkg")));

    delete[] pars_backup_old;
    delete[] pars_backup_new;

    return true;
}

auto FitterFactory::setFlags(PriorityMode new_mode) -> void { d->mode = new_mode; }

auto FitterFactory::setDefaultParameters(HistogramFit* defs) -> void { d->defpars = defs; }

void FitterFactory::setReplacement(const TString& src, const TString& dst)
{
    d->rep_src = src;
    d->rep_dst = dst;
}

void FitterFactory::setNameDecorator(const TString& decorator) { d->name_decorator = decorator; };
void FitterFactory::clearNameDecorator() { d->name_decorator = "*"; };

void FitterFactory::setFunctionDecorator(const TString& decorator)
{
    d->function_decorator = decorator;
};

void FitterFactory::setDrawBits(bool sum, bool sig, bool bkg)
{
    d->draw_sum = sum;
    d->draw_sig = sig;
    d->draw_bkg = bkg;
}

auto FitterFactory::propSum() -> FitterFactoryTools::DrawProperties& { return d->prop_sum; }
auto FitterFactory::propSig() -> FitterFactoryTools::DrawProperties& { return d->prop_sig; }
auto FitterFactory::propBkg() -> FitterFactoryTools::DrawProperties& { return d->prop_bkg; }

void FitterFactory::print() const
{
    for (auto it = d->hfpmap.begin(); it != d->hfpmap.end(); ++it)
        it->second->print();
}

void FitterFactory::clear() { d->hfpmap.clear(); }

TString FitterFactory::format_name(const TString& name, const TString& decorator) const
{
    TString s = decorator;
    s.ReplaceAll("*", name);
    return s;
}

FitterFactoryTools::SelectedSource FitterFactoryTools::selectSource(const char* filename,
                                                                    const char* auxname)
{
#if __cplusplus >= 201703L
    auto s1 = std::filesystem::exists(filename);
    auto s2 = std::filesystem::exists(auxname);

    if (!s1 and !s2) return SelectedSource::None;
    if (s1 and !s2) return SelectedSource::OnlyReference;
    if (!s1 and s2) return SelectedSource::OnlyAuxilary;

    std::filesystem::file_time_type mod_ref = std::filesystem::last_write_time(filename);
    std::filesystem::file_time_type mod_aux = std::filesystem::last_write_time(auxname);
#else
    struct stat st_ref;
    struct stat st_aux;

    auto s1 = stat(filename, &st_ref) == 0;
    auto s2 = stat(auxname, &st_aux) == 0;

    if (!s1 and !s2) return SelectedSource::None;
    if (s1 and !s2) return SelectedSource::OnlyReference;
    if (!s1 and s2) return SelectedSource::OnlyAuxilary;

    auto mod_ref = (long long)st_ref.st_mtim.tv_sec;
    auto mod_aux = (long long)st_aux.st_mtim.tv_sec;
#endif

    return mod_aux > mod_ref ? SelectedSource::Auxilary : SelectedSource::Reference;
}

void FitterFactoryTools::DrawProperties::applyStyle(TF1* f)
{
#if __cplusplus >= 201703L
    if (line_color) f->SetLineColor(*line_color);
    if (line_width) f->SetLineWidth(*line_width);
    if (line_style) f->SetLineStyle(*line_style);
#else
    if (line_color != -1) f->SetLineColor(line_color);
    if (line_width != -1) f->SetLineWidth(line_width);
    if (line_style != -1) f->SetLineStyle(line_style);
#endif
}

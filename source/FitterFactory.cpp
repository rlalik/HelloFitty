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

#include "parser.hpp"

#include <TF1.h>
#include <TH1.h>
#include <TList.h>
#include <TObjArray.h>
#include <TObjString.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <utility>

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

namespace FF
{

struct FitterFactoryImpl
{
    FitterFactory::PriorityMode mode;
    int version{0};

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
    Tools::DrawProperties prop_sum, prop_sig, prop_bkg;
};

bool FitterFactoryImpl::verbose_flag = true;

void Param::print() const
{
    printf("%10g   Mode: %-5s   Limits: ", value, mode == FitMode::Free ? "Free" : "Fixed");
    if (has_limits)
        printf(" %g, %g\n", lower, upper);
    else
        printf(" none\n");
}

struct HistogramFitImpl
{
    TString hist_name;  // histogram name
    TString sig_string; // signal and background functions
    TString bkg_string;

    Double_t range_l; // function range
    Double_t range_u; // function range

    int rebin{0}; // rebin, 0 == no rebin
    bool fit_disabled{false};

    std::vector<Param> pars;
    TF1 function_sig;
    TF1 function_bkg;
    TF1 function_sum;

    std::vector<Double_t> backup_p; // backup for parameters
};

HistogramFit::HistogramFit(TString hist_name, TString formula_s, TString formula_b,
                           Double_t range_lower, Double_t range_upper)
    : d{make_unique<HistogramFitImpl>()}
{
    d->range_l = range_lower;
    d->range_u = range_upper;
    d->function_sig = TF1("", formula_s, range_lower, range_upper, TF1::EAddToList::kNo);
    d->function_bkg = TF1("", formula_b, range_lower, range_upper, TF1::EAddToList::kNo);
    d->function_sum =
        TF1("", formula_s + "+" + formula_b, range_lower, range_upper, TF1::EAddToList::kNo);
    d->hist_name = std::move(hist_name);
    d->sig_string = std::move(formula_s);
    d->bkg_string = std::move(formula_b);

    if (d->hist_name[0] == '@') { d->fit_disabled = true; }

    d->pars.resize(d->function_sum.GetNpar());
}

HistogramFit::~HistogramFit() noexcept = default;

auto HistogramFit::clone(TString new_name) const -> std::unique_ptr<HistogramFit>
{
    return make_unique<HistogramFit>(std::move(new_name), d->sig_string, d->bkg_string, d->range_l,
                                     d->range_u);
}

void HistogramFit::clear() { drop(); }

void HistogramFit::init()
{
    for (auto i = 0; i < d->pars.size(); ++i)
        if (d->pars[i].mode == Param::FitMode::Fixed)
            d->function_sum.FixParameter(i, d->pars[i].value);
        else
        {
            d->function_sum.SetParameter(i, d->pars[i].value);
            if (d->pars[i].has_limits)
                d->function_sum.SetParLimits(i, d->pars[i].lower, d->pars[i].upper);
        }
}

void HistogramFit::setParam(Int_t par, Param value)
{
    if (!(par < d->pars.size())) throw;
    d->pars[par] = value;
}

void HistogramFit::setParam(Int_t par, Double_t val, Param::FitMode mode)
{
    if (!(par < d->pars.size())) throw;
    d->pars[par].value = val;
    d->pars[par].lower = 0;
    d->pars[par].upper = 0;
    d->pars[par].mode = mode;
    d->pars[par].has_limits = false;
}

void HistogramFit::setParam(Int_t par, Double_t val, Double_t l, Double_t u, Param::FitMode mode)
{
    if (!(par < d->pars.size())) throw;
    d->pars[par].value = val;
    d->pars[par].lower = l;
    d->pars[par].upper = u;
    d->pars[par].mode = mode;
    d->pars[par].has_limits = true;
}

auto HistogramFit::updateParam(Int_t par, Double_t value) -> void
{
    if (!(par < d->pars.size())) throw;
    d->pars[par].value = value;
}

auto HistogramFit::getParam(Int_t par) const -> Param
{
    if (!(par < d->pars.size())) throw;
    return d->pars[par];
}

auto HistogramFit::getParamsNumber() const -> int { return d->pars.size(); }

auto HistogramFit::getName() const -> TString { return d->hist_name; }

auto HistogramFit::getFitRangeL() const -> float { return d->range_l; }

auto HistogramFit::getFitRangeU() const -> float { return d->range_u; }

auto HistogramFit::getSigFunc() const -> const TF1& { return d->function_sig; }

auto HistogramFit::getBkgFunc() const -> const TF1& { return d->function_bkg; }

auto HistogramFit::getSumFunc() const -> const TF1& { return d->function_sum; }

auto HistogramFit::getSigFunc() -> TF1& { return d->function_sig; }

auto HistogramFit::getBkgFunc() -> TF1& { return d->function_bkg; }

auto HistogramFit::getSumFunc() -> TF1& { return d->function_sum; }

auto HistogramFit::getSigString() const -> const TString& { return d->sig_string; }

auto HistogramFit::getBkgString() const -> const TString& { return d->bkg_string; }

auto HistogramFit::getFlagRebin() const -> int { return d->rebin; }

auto HistogramFit::getFlagDisabled() const -> bool { return d->fit_disabled; }

TString HistogramFit::exportEntry() const
{
    TString out = d->fit_disabled ? "@" : " ";

    char sep;

    out = TString::Format("%s%s\t%s %s %d %.0f %.0f", out.Data(), d->hist_name.Data(),
                          d->sig_string.Data(), d->bkg_string.Data(), d->rebin, d->range_l,
                          d->range_u);
    auto limit = d->pars.size();

    for (decltype(limit) i = 0; i < limit; ++i)
    {
        TString v = TString::Format("%g", d->pars[i].value);
        TString l = TString::Format("%g", d->pars[i].lower);
        TString u = TString::Format("%g", d->pars[i].upper);

        switch (d->pars[i].mode)
        {
            case Param::FitMode::Free:
                if (d->pars[i].has_limits)
                    sep = ':';
                else
                    sep = ' ';
                break;
            case Param::FitMode::Fixed:
                if (d->pars[i].has_limits)
                    sep = 'F';
                else
                    sep = 'f';
                break;
        }

        if (d->pars[i].mode == Param::FitMode::Free and d->pars[i].has_limits == 0)
            out += TString::Format(" %s", v.Data());
        else if (d->pars[i].mode == Param::FitMode::Fixed and d->pars[i].has_limits == 0)
            out += TString::Format(" %s %c", v.Data(), sep);
        else
            out += TString::Format(" %s %c %s %s", v.Data(), sep, l.Data(), u.Data());
    }

    return out;
}

void HistogramFit::print(bool detailed) const
{
    std::cout << "## name: " << d->hist_name.Data() << "   rebin: " << d->rebin
              << "   range: " << d->range_l << " -- " << d->range_u
              << "  param num: " << d->pars.size() << "\n";

    auto s = d->pars.size();
    for (decltype(s) i = 0; i < s; ++i)
    {
        std::cout << "   " << i << ": ";
        d->pars[i].print();
    }

    if (detailed)
    {
        std::cout << "+++++++++ SIG function +++++++++" << std::endl;
        d->function_sig.Print("V");
        std::cout << "+++++++++ BKG function +++++++++" << std::endl;
        d->function_bkg.Print("V");
        std::cout << "+++++++++ SUM function +++++++++" << std::endl;
        d->function_sum.Print("V");
        std::cout << "++++++++++++++++++++++++++++++++" << std::endl;
    }
}

bool HistogramFit::load(TF1* f)
{
    auto s = d->pars.size();
    if (s == f->GetNpar())
    {
        for (int i = 0; i < s; ++i)
            d->pars[i].value = f->GetParameter(i);
    }
    else
        return false;

    return true;
}

void HistogramFit::push()
{
    d->backup_p.clear();
    for (auto& p : d->pars)
        d->backup_p.push_back(p.value);
}

void HistogramFit::pop()
{
    apply();
    drop();
}

void HistogramFit::apply()
{
    if (d->backup_p.size() != d->pars.size()) return;

    auto n = d->pars.size();
    for (decltype(n) i = 0; i < n; ++i)
        d->pars[i].value = d->backup_p[i];
}

void HistogramFit::drop() { d->backup_p.clear(); }

void FitterFactory::setVerbose(bool verbose) { FitterFactoryImpl::verbose_flag = verbose; }

FitterFactory::FitterFactory(PriorityMode mode) : d{make_unique<FitterFactoryImpl>()}
{
    d->mode = mode;
    d->defpars = nullptr;
}

FitterFactory::~FitterFactory() = default;

bool FitterFactory::initFactoryFromFile(const char* filename, const char* auxname)
{
    d->par_ref = filename;
    d->par_aux = auxname;

    if (!filename) { fprintf(stderr, "No reference input file given\n"); }
    if (!auxname) { fprintf(stderr, "No output file given\n"); }

    auto selected = Tools::selectSource(filename, auxname);

    if (selected == Tools::SelectedSource::None) return false;

    printf(
        "Available source: [%c] REF  [%c] AUX\n",
        selected != Tools::SelectedSource::OnlyAuxiliary and selected != Tools::SelectedSource::None
            ? 'x'
            : ' ',
        selected != Tools::SelectedSource::OnlyReference and selected != Tools::SelectedSource::None
            ? 'x'
            : ' ');
    printf("Selected source : [%c] REF  [%c] AUX\n",
           selected == Tools::SelectedSource::Reference ? 'x' : ' ',
           selected == Tools::SelectedSource::Auxiliary ? 'x' : ' ');

    auto mode = d->mode;
    if (mode == PriorityMode::Reference)
    {
        if (selected == Tools::SelectedSource::OnlyAuxiliary)
            return false;
        else
            return importParameters(filename);
    }

    if (mode == PriorityMode::Auxiliary)
    {
        if (selected == Tools::SelectedSource::OnlyReference)
            return false;
        else
            return importParameters(auxname);
    }

    if (mode == PriorityMode::Newer)
    {
        if (selected == Tools::SelectedSource::Auxiliary or
            selected == Tools::SelectedSource::OnlyAuxiliary)
            return importParameters(auxname);
        else if (selected == Tools::SelectedSource::Reference or
                 selected == Tools::SelectedSource::OnlyReference)
            return importParameters(filename);
    }

    return false;
}

bool FitterFactory::exportFactoryToFile() { return exportParameters(d->par_aux.Data()); }

void FitterFactory::insertParameters(std::unique_ptr<HistogramFit>&& hfp)
{
    insertParameters(hfp->getName(), std::move(hfp));
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
        insertParameters(Tools::parseLineEntry(line, d->version));
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
    auto it = d->hfpmap.find(Tools::format_name(name, d->name_decorator));
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

        auto tmp = d->defpars->clone(Tools::format_name(hist->GetName(), d->name_decorator));
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
    Int_t bin_l = hist->FindBin(hfp->getFitRangeL());
    Int_t bin_u = hist->FindBin(hfp->getFitRangeL());

    hfp->init();

    if (hfp->getFlagRebin() != 0)
    {
        // was_rebin = true;
        hist->Rebin(hfp->getFlagRebin());
    }

    if (hist->Integral(bin_l, bin_u) == 0) return false;

    TF1* tfSig = &hfp->getSigFunc();
    TF1* tfBkg = &hfp->getBkgFunc();
    TF1* tfSum = &hfp->getSumFunc();

    tfSig->SetName(Tools::format_name(hfp->getName(), d->function_decorator + "_sig"));
    tfBkg->SetName(Tools::format_name(hfp->getName(), d->function_decorator + "_bkg"));
    tfSum->SetName(Tools::format_name(hfp->getName(), d->function_decorator));

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
    auto pars_backup_old = make_unique<double[]>(par_num);
    tfSum->GetParameters(pars_backup_old.get());
    double chi2_backup_old = hist->Chisquare(tfSum, "R");

    if (d->verbose_flag)
    {
        // print them
        printf("* old: ");
        for (uint i = 0; i < par_num; ++i)
            printf("%g ", pars_backup_old[i]);
        printf(" --> chi2:  %f -- *\n", chi2_backup_old);
    }

    hist->Fit(tfSum, pars, gpars, hfp->getFitRangeL(), hfp->getFitRangeU());

    TF1* new_sig_func = ((TF1*)hist->GetListOfFunctions()->At(0));

    // TVirtualFitter * fitter = TVirtualFitter::GetFitter();
    // TMatrixDSym cov;
    // fitter->GetCovarianceMatrix()
    // cov.Use(fitter->GetNumberTotalParameters(), fitter->GetCovarianceMatrix());
    // cov.Print();

    // backup new parameters
    auto pars_backup_new = make_unique<double[]>(par_num);
    tfSum->GetParameters(pars_backup_new.get());
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
        tfSum->SetParameters(pars_backup_old.get());
        new_sig_func->SetParameters(pars_backup_old.get());
        printf("\n\tFIT-ERROR: Fit got worse -> restoring params for chi2 = %g",
               hist->Chisquare(tfSum, "R"));
    }
    else if (tfSum->GetMaximum() > 2.0 * hist->GetMaximum())
    {
        tfSum->SetParameters(pars_backup_old.get());
        new_sig_func->SetParameters(pars_backup_old.get());
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

        hfp->updateParam(i, par);
    }

    for (int i = parnsig; i < tfBkg->GetNpar(); ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        tfBkg->SetParameter(i, par);
        tfBkg->SetParError(i, err);

        hfp->updateParam(i, par);
    }

    hist->GetListOfFunctions()->Add(
        tfSig->Clone(Tools::format_name(hfp->getName(), d->function_decorator + "_sig")));
    hist->GetListOfFunctions()->Add(
        tfBkg->Clone(Tools::format_name(hfp->getName(), d->function_decorator + "_bkg")));

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

auto FitterFactory::propSum() -> Tools::DrawProperties& { return d->prop_sum; }
auto FitterFactory::propSig() -> Tools::DrawProperties& { return d->prop_sig; }
auto FitterFactory::propBkg() -> Tools::DrawProperties& { return d->prop_bkg; }

void FitterFactory::print() const
{
    for (auto it = d->hfpmap.begin(); it != d->hfpmap.end(); ++it)
        it->second->print();
}

void FitterFactory::clear() { d->hfpmap.clear(); }

}; // namespace FF

namespace FF::Tools
{
SelectedSource selectSource(const char* filename, const char* auxname)
{
#if __cplusplus >= 201703L
    auto s1 = std::filesystem::exists(filename);
    auto s2 = std::filesystem::exists(auxname);

    if (!s1 and !s2) return SelectedSource::None;
    if (s1 and !s2) return SelectedSource::OnlyReference;
    if (!s1 and s2) return SelectedSource::OnlyAuxiliary;

    std::filesystem::file_time_type mod_ref = std::filesystem::last_write_time(filename);
    std::filesystem::file_time_type mod_aux = std::filesystem::last_write_time(auxname);
#else
    struct stat st_ref;
    struct stat st_aux;

    auto s1 = stat(filename, &st_ref) == 0;
    auto s2 = stat(auxname, &st_aux) == 0;

    if (!s1 and !s2) return SelectedSource::None;
    if (s1 and !s2) return SelectedSource::OnlyReference;
    if (!s1 and s2) return SelectedSource::OnlyAuxiliary;

    auto mod_ref = (long long)st_ref.st_mtim.tv_sec;
    auto mod_aux = (long long)st_aux.st_mtim.tv_sec;
#endif

    return mod_aux > mod_ref ? SelectedSource::Auxiliary : SelectedSource::Reference;
}

void DrawProperties::applyStyle(TF1* f)
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

TString format_name(const TString& name, const TString& decorator)
{
    TString s = decorator;
    s.ReplaceAll("*", name);
    return s;
}

std::unique_ptr<HistogramFit> parseLineEntry(const TString& line, int version)
{
    return parseLineEntry_v1(line);
}

} // namespace FF::Tools

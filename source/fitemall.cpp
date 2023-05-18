/*
    FitemAll - a versatile histogram fitting tool for ROOT-based projects
    Copyright (C) 2015-2023  Rafał Lalik <rafallalik@gmail.com>

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

#include "fitemall.hpp"

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

namespace fea
{

namespace detail
{
struct fitter_impl
{
    fitter::priority_mode mode;
    int version{0};

    static bool verbose_flag;

    TString par_ref;
    TString par_aux;

    histogram_fit* defpars{nullptr};
    std::map<TString, std::unique_ptr<histogram_fit>> hfpmap;

    TString name_decorator{"*"};
    TString function_decorator{"f_*"};

    TString rep_src;
    TString rep_dst;

    bool draw_sum{true}, draw_sig{false}, draw_bkg{false};
    tools::draw_properties prop_sum, prop_sig, prop_bkg;
};

bool fitter_impl::verbose_flag = true;
} // namespace detail

void param::print() const
{
    printf("%10g   Mode: %-5s   Limits: ", value, mode == fit_mode::free ? "free" : "fixed");
    if (has_limits)
        printf(" %g, %g\n", lower, upper);
    else
        printf(" none\n");
}

namespace detail
{
struct histogram_fit_impl
{
    TString hist_name;  // histogram name
    TString sig_string; // signal and background functions
    TString bkg_string;

    Double_t range_l; // function range
    Double_t range_u; // function range

    int rebin{0}; // rebin, 0 == no rebin
    bool fit_disabled{false};

    std::vector<param> pars;
    TF1 function_sig;
    TF1 function_bkg;
    TF1 function_sum;

    std::vector<Double_t> backup_p; // backup for parameters
};
} // namespace detail

histogram_fit::histogram_fit(TString hist_name, TString formula_s, TString formula_b,
                             Double_t range_lower, Double_t range_upper)
    : d{make_unique<detail::histogram_fit_impl>()}
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

histogram_fit::~histogram_fit() noexcept = default;

auto histogram_fit::clone(TString new_name) const -> std::unique_ptr<histogram_fit>
{
    return make_unique<histogram_fit>(std::move(new_name), d->sig_string, d->bkg_string, d->range_l,
                                      d->range_u);
}

void histogram_fit::clear() { drop(); }

void histogram_fit::init()
{
    for (auto i = 0; i < d->pars.size(); ++i)
        if (d->pars[i].mode == param::fit_mode::fixed)
            d->function_sum.FixParameter(i, d->pars[i].value);
        else
        {
            d->function_sum.SetParameter(i, d->pars[i].value);
            if (d->pars[i].has_limits)
                d->function_sum.SetParLimits(i, d->pars[i].lower, d->pars[i].upper);
        }
}

void histogram_fit::set_param(Int_t par, param value)
{
    if (!(par < d->pars.size())) throw;
    d->pars[par] = value;
}

void histogram_fit::set_param(Int_t par, Double_t val, param::fit_mode mode)
{
    if (!(par < d->pars.size())) throw;
    d->pars[par].value = val;
    d->pars[par].lower = 0;
    d->pars[par].upper = 0;
    d->pars[par].mode = mode;
    d->pars[par].has_limits = false;
}

void histogram_fit::set_param(Int_t par, Double_t val, Double_t l, Double_t u, param::fit_mode mode)
{
    if (!(par < d->pars.size())) throw;
    d->pars[par].value = val;
    d->pars[par].lower = l;
    d->pars[par].upper = u;
    d->pars[par].mode = mode;
    d->pars[par].has_limits = true;
}

auto histogram_fit::update_param(Int_t par, Double_t value) -> void
{
    if (!(par < d->pars.size())) throw;
    d->pars[par].value = value;
}

auto histogram_fit::get_param(Int_t par) const -> param
{
    if (!(par < d->pars.size())) throw;
    return d->pars[par];
}

auto histogram_fit::get_params_number() const -> int { return d->pars.size(); }

auto histogram_fit::get_name() const -> TString { return d->hist_name; }

auto histogram_fit::get_fit_range_l() const -> double { return d->range_l; }

auto histogram_fit::get_fit_range_u() const -> double { return d->range_u; }

auto histogram_fit::get_sig_func() const -> const TF1& { return d->function_sig; }

auto histogram_fit::get_bkg_func() const -> const TF1& { return d->function_bkg; }

auto histogram_fit::get_sum_func() const -> const TF1& { return d->function_sum; }

auto histogram_fit::get_sig_func() -> TF1& { return d->function_sig; }

auto histogram_fit::get_bkg_func() -> TF1& { return d->function_bkg; }

auto histogram_fit::get_sum_func() -> TF1& { return d->function_sum; }

auto histogram_fit::get_sig_string() const -> const TString& { return d->sig_string; }

auto histogram_fit::get_bkg_string() const -> const TString& { return d->bkg_string; }

auto histogram_fit::get_flag_rebin() const -> int { return d->rebin; }

auto histogram_fit::get_flag_disabled() const -> bool { return d->fit_disabled; }

TString histogram_fit::export_entry() const
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
            case param::fit_mode::free:
                if (d->pars[i].has_limits)
                    sep = ':';
                else
                    sep = ' ';
                break;
            case param::fit_mode::fixed:
                if (d->pars[i].has_limits)
                    sep = 'F';
                else
                    sep = 'f';
                break;
        }

        if (d->pars[i].mode == param::fit_mode::free and d->pars[i].has_limits == 0)
            out += TString::Format(" %s", v.Data());
        else if (d->pars[i].mode == param::fit_mode::fixed and d->pars[i].has_limits == 0)
            out += TString::Format(" %s %c", v.Data(), sep);
        else
            out += TString::Format(" %s %c %s %s", v.Data(), sep, l.Data(), u.Data());
    }

    return out;
}

void histogram_fit::print(bool detailed) const
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

bool histogram_fit::load(TF1* f)
{
    auto s = d->pars.size();
    if (s == f->GetNpar())
    {
        for (uint i = 0; i < s; ++i)
            d->pars[i].value = f->GetParameter(i);
    }
    else
        return false;

    return true;
}

void histogram_fit::push()
{
    d->backup_p.clear();
    for (auto& p : d->pars)
        d->backup_p.push_back(p.value);
}

void histogram_fit::pop()
{
    apply();
    drop();
}

void histogram_fit::apply()
{
    if (d->backup_p.size() != d->pars.size()) return;

    auto n = d->pars.size();
    for (decltype(n) i = 0; i < n; ++i)
        d->pars[i].value = d->backup_p[i];
}

void histogram_fit::drop() { d->backup_p.clear(); }

void fitter::set_verbose(bool verbose) { detail::fitter_impl::verbose_flag = verbose; }

fitter::fitter(priority_mode mode) : d{make_unique<detail::fitter_impl>()}
{
    d->mode = mode;
    d->defpars = nullptr;
}

fitter::~fitter() = default;

bool fitter::init_fitter_from_file(const char* filename, const char* auxname)
{
    d->par_ref = filename;
    d->par_aux = auxname;

    if (!filename) { fprintf(stderr, "No reference input file given\n"); }
    if (!auxname) { fprintf(stderr, "No output file given\n"); }

    auto selected = tools::select_source(filename, auxname);

    if (selected == tools::selected_source::none) return false;

    printf("Available source: [%c] REF  [%c] AUX\n",
           selected != tools::selected_source::only_auxiliary and
                   selected != tools::selected_source::none
               ? 'x'
               : ' ',
           selected != tools::selected_source::only_reference and
                   selected != tools::selected_source::none
               ? 'x'
               : ' ');
    printf("Selected source : [%c] REF  [%c] AUX\n",
           selected == tools::selected_source::reference ? 'x' : ' ',
           selected == tools::selected_source::auxiliary ? 'x' : ' ');

    auto mode = d->mode;
    if (mode == priority_mode::reference)
    {
        if (selected == tools::selected_source::only_auxiliary)
            return false;
        else
            return import_parameters(filename);
    }

    if (mode == priority_mode::auxiliary)
    {
        if (selected == tools::selected_source::only_reference)
            return false;
        else
            return import_parameters(auxname);
    }

    if (mode == priority_mode::newer)
    {
        if (selected == tools::selected_source::auxiliary or
            selected == tools::selected_source::only_auxiliary)
            return import_parameters(auxname);
        else if (selected == tools::selected_source::reference or
                 selected == tools::selected_source::only_reference)
            return import_parameters(filename);
    }

    return false;
}

bool fitter::export_fitter_to_file() { return export_parameters(d->par_aux.Data()); }

void fitter::insert_parameters(std::unique_ptr<histogram_fit>&& hfp)
{
    insert_parameters(hfp->get_name(), std::move(hfp));
}

void fitter::insert_parameters(const TString& name, std::unique_ptr<histogram_fit>&& hfp)
{
    d->hfpmap.insert({name, std::move(hfp)});
}

bool fitter::import_parameters(const std::string& filename)
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
        insert_parameters(tools::parse_line_entry(line, d->version));
    }

    return true;
}

bool fitter::export_parameters(const std::string& filename)
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
            fparfile << it->second->export_entry().Data() << std::endl;
        }
    }
    fparfile.close();
    return true;
}

histogram_fit* fitter::find_fit(TH1* hist) const { return find_fit(hist->GetName()); }

histogram_fit* fitter::find_fit(const char* name) const
{
    auto it = d->hfpmap.find(tools::format_name(name, d->name_decorator));
    if (it != d->hfpmap.end()) return it->second.get();

    return nullptr;
}

bool fitter::fit(TH1* hist, const char* pars, const char* gpars)
{
    histogram_fit* hfp = find_fit(hist->GetName());
    if (!hfp)
    {
        printf("HFP for histogram %s not found, trying from defaults.\n", hist->GetName());

        if (!d->defpars) return false;

        auto tmp = d->defpars->clone(tools::format_name(hist->GetName(), d->name_decorator));
        hfp = tmp.get();
        insert_parameters(std::move(tmp));
    }

    hfp->push();
    bool status = fit(hfp, hist, pars, gpars);

    if (!status) hfp->pop();

    return status;
}

bool fitter::fit(histogram_fit* hfp, TH1* hist, const char* pars, const char* gpars)
{
    Int_t bin_l = hist->FindBin(hfp->get_fit_range_l());
    Int_t bin_u = hist->FindBin(hfp->get_fit_range_u());

    hfp->init();

    if (hfp->get_flag_rebin() != 0)
    {
        // was_rebin = true;
        hist->Rebin(hfp->get_flag_rebin());
    }

    if (hist->Integral(bin_l, bin_u) == 0) return false;

    TF1* tfSig = &hfp->get_sig_func();
    TF1* tfBkg = &hfp->get_bkg_func();
    TF1* tfSum = &hfp->get_sum_func();

    tfSig->SetName(tools::format_name(hfp->get_name(), d->function_decorator + "_sig"));
    tfBkg->SetName(tools::format_name(hfp->get_name(), d->function_decorator + "_bkg"));
    tfSum->SetName(tools::format_name(hfp->get_name(), d->function_decorator));

    hist->GetListOfFunctions()->Clear();
    hist->GetListOfFunctions()->SetOwner(kTRUE);

    if (d->draw_sig)
        prop_sig().apply_style(tfSig);
    else
        tfSig->SetBit(TF1::kNotDraw);
    // tfSig->SetBit(TF1::kNotGlobal);
    if (d->draw_bkg)
        prop_bkg().apply_style(tfBkg);
    else
        tfBkg->SetBit(TF1::kNotDraw);
    // tfBkg->SetBit(TF1::kNotGlobal);
    if (d->draw_sum)
        prop_sum().apply_style(tfSum);
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

    hist->Fit(tfSum, pars, gpars, hfp->get_fit_range_l(), hfp->get_fit_range_u());

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

    auto parnsig = tfSig->GetNpar();
    for (decltype(parnsig) i = 0; i < parnsig; ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        tfSig->SetParameter(i, par);
        tfSig->SetParError(i, err);

        hfp->update_param(i, par);
    }

    for (auto i = parnsig; i < tfBkg->GetNpar(); ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        tfBkg->SetParameter(i, par);
        tfBkg->SetParError(i, err);

        hfp->update_param(i, par);
    }

    hist->GetListOfFunctions()->Add(
        tfSig->Clone(tools::format_name(hfp->get_name(), d->function_decorator + "_sig")));
    hist->GetListOfFunctions()->Add(
        tfBkg->Clone(tools::format_name(hfp->get_name(), d->function_decorator + "_bkg")));

    return true;
}

auto fitter::set_flags(priority_mode new_mode) -> void { d->mode = new_mode; }

auto fitter::set_default_parameters(histogram_fit* defs) -> void { d->defpars = defs; }

void fitter::set_replacement(const TString& src, const TString& dst)
{
    d->rep_src = src;
    d->rep_dst = dst;
}

void fitter::set_name_decorator(const TString& decorator) { d->name_decorator = decorator; };
void fitter::clear_name_decorator() { d->name_decorator = "*"; };

void fitter::set_function_decorator(const TString& decorator)
{
    d->function_decorator = decorator;
};

void fitter::set_draw_bits(bool sum, bool sig, bool bkg)
{
    d->draw_sum = sum;
    d->draw_sig = sig;
    d->draw_bkg = bkg;
}

auto fitter::prop_sum() -> tools::draw_properties& { return d->prop_sum; }
auto fitter::prop_sig() -> tools::draw_properties& { return d->prop_sig; }
auto fitter::prop_bkg() -> tools::draw_properties& { return d->prop_bkg; }

void fitter::print() const
{
    for (auto it = d->hfpmap.begin(); it != d->hfpmap.end(); ++it)
        it->second->print();
}

void fitter::clear() { d->hfpmap.clear(); }

namespace tools
{
selected_source select_source(const char* filename, const char* auxname)
{
#if __cplusplus >= 201703L
    auto s1 = std::filesystem::exists(filename);
    auto s2 = std::filesystem::exists(auxname);

    if (!s1 and !s2) return selected_source::none;
    if (s1 and !s2) return selected_source::only_reference;
    if (!s1 and s2) return selected_source::only_auxiliary;

    std::filesystem::file_time_type mod_ref = std::filesystem::last_write_time(filename);
    std::filesystem::file_time_type mod_aux = std::filesystem::last_write_time(auxname);
#else
    struct stat st_ref;
    struct stat st_aux;

    auto s1 = stat(filename, &st_ref) == 0;
    auto s2 = stat(auxname, &st_aux) == 0;

    if (!s1 and !s2) return selected_source::none;
    if (s1 and !s2) return selected_source::only_reference;
    if (!s1 and s2) return selected_source::only_auxiliary;

    auto mod_ref = (long long)st_ref.st_mtim.tv_sec;
    auto mod_aux = (long long)st_aux.st_mtim.tv_sec;
#endif

    return mod_aux > mod_ref ? selected_source::auxiliary : selected_source::reference;
}

void draw_properties::apply_style(TF1* f)
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

std::unique_ptr<histogram_fit> parse_line_entry(const TString& line, int /*version*/)
{
    return parse_line_entry_v1(line);
}

} // namespace tools

} // namespace fea

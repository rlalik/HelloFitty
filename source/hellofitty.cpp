/*
    HelloFitty - a versatile histogram fitting tool for ROOT-based projects
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

#include "hellofitty.hpp"

#include "parser.hpp"

#include <TF1.h>
#include <TH1.h>
#include <TList.h>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <sys/stat.h>
#endif

#if __cplusplus < 201402L
template <typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
using std::make_unique;
#endif

struct params_vector
{
    std::vector<double> pars;
    params_vector(size_t n) : pars(n) {}
};

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <> struct fmt::formatter<params_vector>
{
    // Presentation format: 'f' - fixed, 'e' - exponential, 'g' - either.
    char presentation = 'f';

    // Parses format specifications of the form ['f' | 'e' | 'g'].
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
    {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e' || *it == 'g')) presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}') ctx.on_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Formats the point p using the parsed format specification (presentation)
    // stored in this formatter.
    auto format(const params_vector& p, format_context& ctx) const -> format_context::iterator
    {
        // ctx.out() is an output iterator to write to.

        if (presentation == 'f')
            for (const auto& par : p.pars)
                fmt::format_to(ctx.out(), "{:f} ", par);
        else if (presentation == 'e')
            for (const auto& par : p.pars)
                fmt::format_to(ctx.out(), "{:e} ", par);
        else if (presentation == 'g')
            for (const auto& par : p.pars)
                fmt::format_to(ctx.out(), "{:g} ", par);
        else
            throw format_error("invalid format specifier");

        return ctx.out();
    }
};

template <> struct fmt::formatter<hf::param>
{
    // Presentation format: 'f' - fixed, 'e' - exponential, 'g' - either.
    char presentation = 'g';

    // Parses format specifications of the form ['f' | 'e' | 'g'].
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
    {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e' || *it == 'g')) presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}') ctx.on_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    auto format(const hf::param& par, format_context& ctx) const -> format_context::iterator
    {
        char sep{0};

        switch (par.mode)
        {
            case hf::param::fit_mode::free:
                if (par.has_limits) { sep = ':'; }
                else { sep = ' '; }
                break;
            case hf::param::fit_mode::fixed:
                if (par.has_limits) { sep = 'F'; }
                else { sep = 'f'; }
                break;
            default:
                throw std::runtime_error("Unknown hf::param mode");
                break;
        }

        if (par.mode == hf::param::fit_mode::free and par.has_limits == false)
        {
            fmt::format_to(ctx.out(), " {:g} ", par.value);
        }
        else if (par.mode == hf::param::fit_mode::fixed and par.has_limits == false)
        {
            fmt::format_to(ctx.out(), " {:g} {:c}", par.value, sep);
        }
        else
        {
            fmt::format_to(ctx.out(), " {:g} {:c} {:g} {:g}", par.value, sep, par.min, par.max);

            return ctx.out();
        }
        
        return ctx.out();
    }
};

template <> struct fmt::formatter<hf::fit_entry>
{
    // Presentation format: 'f' - fixed, 'e' - exponential, 'g' - either.
    char presentation = 'g';

    // Parses format specifications of the form ['f' | 'e' | 'g'].
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
    {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e' || *it == 'g')) presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}') ctx.on_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    auto format(const hf::fit_entry& fitentry, format_context& ctx) const -> format_context::iterator
    {
        return ctx.out();
    }
};

namespace hf
{

/// Based on
/// https://stackoverflow.com/questions/27490762/how-can-i-convert-to-size-t-from-int-safely
constexpr auto size_t2int(size_t val) -> int
{
    return (val <= std::numeric_limits<int>::max()) ? static_cast<int>(val) : -1;
}
constexpr auto int2size_t(int val) -> size_t { return (val < 0) ? __SIZE_MAX__ : static_cast<size_t>(val); }

auto param::print() const -> void
{
    fmt::print("{:10g}   Mode: {:>5}   Limits: ", value, mode == fit_mode::free ? "free" : "fixed");
    if (has_limits)
        fmt::print(" {:g}, {:g}\n", min, max);
    else
        fmt::print(" none\n");
}

namespace detail
{
struct fitter_impl
{
    fitter::priority_mode mode;
    int version{0};

    static bool verbose_flag;

    TString par_ref;
    TString par_aux;

    fit_entry* defpars{nullptr};
    std::map<TString, std::unique_ptr<fit_entry>> hfpmap;

    TString name_decorator{"*"};
    TString function_decorator{"f_*"};

    TString rep_src;
    TString rep_dst;

    bool draw_sum{true};
    bool draw_sig{false};
    bool draw_bkg{false};
    tools::draw_properties prop_sum, prop_sig, prop_bkg;
};

bool fitter_impl::verbose_flag = true;

struct fit_entry_impl
{
    TString hist_name;  // histogram name
    TString sig_string; // signal and background functions
    TString bkg_string;

    Double_t range_min; // function range
    Double_t range_max; // function range

    int rebin{0}; // rebin, 0 == no rebin
    bool fit_disabled{false};

    std::vector<param> pars;
    TF1 function_sig;
    TF1 function_bkg;
    TF1 function_sum;

    std::vector<Double_t> backup_p; // backup for parameters
};
} // namespace detail

fit_entry::fit_entry(TString hist_name, TString formula_s, TString formula_b, Double_t range_min, Double_t range_max)
    : m_d{make_unique<detail::fit_entry_impl>()}
{
    m_d->range_min = range_min;
    m_d->range_max = range_max;
    m_d->function_sig = TF1("", formula_s, range_min, range_max, TF1::EAddToList::kNo);
    m_d->function_bkg = TF1("", formula_b, range_min, range_max, TF1::EAddToList::kNo);
    m_d->function_sum = TF1("", formula_s + "+" + formula_b, range_min, range_max, TF1::EAddToList::kNo);
    m_d->hist_name = std::move(hist_name);
    m_d->sig_string = std::move(formula_s);
    m_d->bkg_string = std::move(formula_b);

    if (m_d->hist_name[0] == '@') { m_d->fit_disabled = true; }

    m_d->pars.resize(int2size_t(m_d->function_sum.GetNpar()));
}

fit_entry::~fit_entry() noexcept = default;

auto fit_entry::clone(TString new_name) const -> std::unique_ptr<fit_entry>
{
    return make_unique<fit_entry>(std::move(new_name), m_d->sig_string, m_d->bkg_string, m_d->range_min,
                                  m_d->range_max);
}

auto fit_entry::clear() -> void { drop(); }

auto fit_entry::init() -> void
{
    for (size_t i = 0; i < m_d->pars.size(); ++i)
        if (m_d->pars[i].mode == param::fit_mode::fixed)
            m_d->function_sum.FixParameter(size_t2int(i), m_d->pars[i].value);
        else
        {
            m_d->function_sum.SetParameter(size_t2int(i), m_d->pars[i].value);
            if (m_d->pars[i].has_limits)
                m_d->function_sum.SetParLimits(size_t2int(i), m_d->pars[i].min, m_d->pars[i].max);
        }
}

auto fit_entry::set_param(Int_t par, param value) -> void
{
    const auto upar = int2size_t(par);
    if (!(upar < m_d->pars.size())) throw;
    m_d->pars[upar] = value;
}

auto fit_entry::set_param(Int_t par, Double_t val, param::fit_mode mode) -> void
{
    const auto upar = int2size_t(par);
    if (!(upar < m_d->pars.size())) throw;
    m_d->pars[upar].value = val;
    m_d->pars[upar].min = 0;
    m_d->pars[upar].max = 0;
    m_d->pars[upar].mode = mode;
    m_d->pars[upar].has_limits = false;
}

auto fit_entry::set_param(Int_t par, Double_t val, Double_t l, Double_t u, param::fit_mode mode) -> void
{
    const auto upar = int2size_t(par);
    if (!(upar < m_d->pars.size())) throw;
    m_d->pars[upar].value = val;
    m_d->pars[upar].min = l;
    m_d->pars[upar].max = u;
    m_d->pars[upar].mode = mode;
    m_d->pars[upar].has_limits = true;
}

auto fit_entry::update_param(Int_t par, Double_t value) -> void
{
    const auto upar = int2size_t(par);
    if (!(upar < m_d->pars.size())) throw;
    m_d->pars[upar].value = value;
}

auto fit_entry::get_param(Int_t par) const -> param
{
    const auto upar = int2size_t(par);
    if (!(upar < m_d->pars.size())) throw;
    return m_d->pars[upar];
}

auto fit_entry::get_params_number() const -> Int_t { return size_t2int(m_d->pars.size()); }

auto fit_entry::get_name() const -> TString { return m_d->hist_name; }

auto fit_entry::get_fit_range_min() const -> Double_t { return m_d->range_min; }

auto fit_entry::get_fit_range_max() const -> Double_t { return m_d->range_max; }

auto fit_entry::get_sig_func() const -> const TF1& { return m_d->function_sig; }

auto fit_entry::get_bkg_func() const -> const TF1& { return m_d->function_bkg; }

auto fit_entry::get_sum_func() const -> const TF1& { return m_d->function_sum; }

auto fit_entry::get_sig_func() -> TF1& { return m_d->function_sig; }

auto fit_entry::get_bkg_func() -> TF1& { return m_d->function_bkg; }

auto fit_entry::get_sum_func() -> TF1& { return m_d->function_sum; }

auto fit_entry::get_sig_string() const -> const TString& { return m_d->sig_string; }

auto fit_entry::get_bkg_string() const -> const TString& { return m_d->bkg_string; }

auto fit_entry::get_flag_rebin() const -> int { return m_d->rebin; }

auto fit_entry::get_flag_disabled() const -> bool { return m_d->fit_disabled; }

auto fit_entry::export_entry() const -> TString { return parser::format_line_entry_v1(this); }

auto fit_entry::print(bool detailed) const -> void
{
    fmt::print("## name: {:s}    rebin: {:d}   range: {:g} -- {:g}  param num: {:d}\n", m_d->hist_name.Data(),
               m_d->rebin, m_d->range_min, m_d->range_min, m_d->pars.size());

    auto s = m_d->pars.size();
    for (decltype(s) i = 0; i < s; ++i)
    {
        fmt::print("   {}: ", i);
        m_d->pars[i].print();
    }

    if (detailed)
    {
        fmt::print("{}\n", "+++++++++ SIG function +++++++++");
        m_d->function_sig.Print("V");
        fmt::print("{}\n", "+++++++++ BKG function +++++++++");
        m_d->function_bkg.Print("V");
        fmt::print("{}\n", "+++++++++ SUM function +++++++++");
        m_d->function_sum.Print("V");
        fmt::print("{}\n", "++++++++++++++++++++++++++++++++");
    }
}

auto fit_entry::load(TF1* function) -> bool
{
    const auto s = m_d->pars.size();
    if (size_t2int(s) == function->GetNpar())
    {
        for (std::remove_const<decltype(s)>::type i = 0; i < s; ++i)
        {
            m_d->pars[i].value = function->GetParameter(size_t2int(i));
        }
    }
    else
        return false;

    return true;
}

auto fit_entry::backup() -> void
{
    m_d->backup_p.clear();
    for (auto& p : m_d->pars)
    {
        m_d->backup_p.push_back(p.value);
    }
}

auto fit_entry::restore() -> void
{
    if (m_d->backup_p.size() != m_d->pars.size()) throw std::out_of_range("Backup storage is empty.");

    const auto n = m_d->pars.size();
    for (std::remove_const<decltype(n)>::type i = 0; i < n; ++i)
    {
        m_d->pars[i].value = m_d->backup_p[i];
    }
}

auto fit_entry::drop() -> void { m_d->backup_p.clear(); }

auto fitter::set_verbose(bool verbose) -> void { detail::fitter_impl::verbose_flag = verbose; }

fitter::fitter(priority_mode mode) : m_d{make_unique<detail::fitter_impl>()}
{
    m_d->mode = mode;
    m_d->defpars = nullptr;
}

fitter::~fitter() = default;

auto fitter::init_fitter_from_file(const char* filename, const char* auxname) -> bool
{
    m_d->par_ref = filename;
    m_d->par_aux = auxname;

    if (!filename) { fprintf(stderr, "No reference input file given\n"); }
    if (!auxname) { fprintf(stderr, "No output file given\n"); }

    auto selected = tools::select_source(filename, auxname);

    if (selected == tools::source::none) return false;

    fmt::print("Available source: [{:c}] REF  [{:c}] AUX\n",
               selected != tools::source::only_auxiliary and selected != tools::source::none ? 'x' : ' ',
               selected != tools::source::only_reference and selected != tools::source::none ? 'x' : ' ');
    fmt::print("Selected source : [{:c}] REF  [{:c}] AUX\n", selected == tools::source::reference ? 'x' : ' ',
               selected == tools::source::auxiliary ? 'x' : ' ');

    auto mode = m_d->mode;
    if (mode == priority_mode::reference)
    {
        if (selected == tools::source::only_auxiliary)
            return false;
        else
            return import_parameters(filename);
    }

    if (mode == priority_mode::auxiliary)
    {
        if (selected == tools::source::only_reference)
            return false;
        else
            return import_parameters(auxname);
    }

    if (mode == priority_mode::newer)
    {
        if (selected == tools::source::auxiliary or selected == tools::source::only_auxiliary)
            return import_parameters(auxname);
        else if (selected == tools::source::reference or selected == tools::source::only_reference)
            return import_parameters(filename);
    }

    return false;
}

auto fitter::export_fitter_to_file() -> bool { return export_parameters(m_d->par_aux.Data()); }

auto fitter::insert_parameters(std::unique_ptr<fit_entry>&& hfp) -> void
{
    insert_parameters(hfp->get_name(), std::move(hfp));
}

auto fitter::insert_parameters(const TString& name, std::unique_ptr<fit_entry>&& hfp) -> void
{
    m_d->hfpmap.insert({name, std::move(hfp)});
}

auto fitter::import_parameters(const std::string& filename) -> bool
{
    std::ifstream fparfile(filename);
    if (!fparfile.is_open())
    {
        fmt::print(stderr, "No file {} to open.\n", filename);
        return false;
    }

    m_d->hfpmap.clear();

    std::string line;
    while (std::getline(fparfile, line))
    {
        insert_parameters(tools::parse_line_entry(line, m_d->version));
    }

    return true;
}

auto fitter::export_parameters(const std::string& filename) -> bool
{
    std::ofstream fparfile(filename);
    if (!fparfile.is_open()) { fmt::print(stderr, "Can't create AUX file {}. Skipping...\n", filename); }
    else
    {
        fmt::print("AUX file {} opened...  Exporting {} entries.\n", filename, m_d->hfpmap.size());
        for (auto it = m_d->hfpmap.begin(); it != m_d->hfpmap.end(); ++it)
        {
            fparfile << it->second->export_entry().Data() << std::endl;
        }
    }
    fparfile.close();
    return true;
}

auto fitter::find_fit(TH1* hist) const -> fit_entry* { return find_fit(hist->GetName()); }

auto fitter::find_fit(const char* name) const -> fit_entry*
{
    auto it = m_d->hfpmap.find(tools::format_name(name, m_d->name_decorator));
    if (it != m_d->hfpmap.end()) return it->second.get();

    return nullptr;
}

auto fitter::fit(TH1* hist, const char* pars, const char* gpars) -> bool
{
    fit_entry* hfp = find_fit(hist->GetName());
    if (!hfp)
    {
        fmt::print("HFP for histogram {:s} not found, trying from defaults.\n", hist->GetName());

        if (!m_d->defpars) return false;

        auto tmp = m_d->defpars->clone(tools::format_name(hist->GetName(), m_d->name_decorator));
        hfp = tmp.get();
        insert_parameters(std::move(tmp));
    }

    hfp->backup();
    bool status = fit(hfp, hist, pars, gpars);

    if (!status) hfp->restore();

    return status;
}

auto fitter::fit(fit_entry* hfp, TH1* hist, const char* pars, const char* gpars) -> bool
{
    Int_t bin_l = hist->FindBin(hfp->get_fit_range_min());
    Int_t bin_u = hist->FindBin(hfp->get_fit_range_max());

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

    tfSig->SetName(tools::format_name(hfp->get_name(), m_d->function_decorator + "_sig"));
    tfBkg->SetName(tools::format_name(hfp->get_name(), m_d->function_decorator + "_bkg"));
    tfSum->SetName(tools::format_name(hfp->get_name(), m_d->function_decorator));

    hist->GetListOfFunctions()->Clear();
    hist->GetListOfFunctions()->SetOwner(kTRUE);

    if (m_d->draw_sig) { prop_sig().apply_style(tfSig); }
    else { tfSig->SetBit(TF1::kNotDraw); }
    // tfSig->SetBit(TF1::kNotGlobal);
    if (m_d->draw_bkg) { prop_bkg().apply_style(tfBkg); }
    else { tfBkg->SetBit(TF1::kNotDraw); }
    // tfBkg->SetBit(TF1::kNotGlobal);
    if (m_d->draw_sum) { prop_sum().apply_style(tfSum); }
    else { tfSum->SetBit(TF1::kNotDraw); }
    // tfSum->SetBit(TF1::kNotGlobal);

    const auto par_num = tfSum->GetNpar();

    // backup old parameters
    params_vector backup_old(int2size_t(par_num));
    tfSum->GetParameters(backup_old.pars.data());
    double chi2_backup_old = hist->Chisquare(tfSum, "R");

    if (m_d->verbose_flag) { fmt::print("* old: {:g} --> chi2:  {:f} -- *\n", backup_old, chi2_backup_old); }

    hist->Fit(tfSum, pars, gpars, hfp->get_fit_range_min(), hfp->get_fit_range_max());

    TF1* new_sig_func = dynamic_cast<TF1*>(hist->GetListOfFunctions()->At(0));

    // TVirtualFitter * fitter = TVirtualFitter::GetFitter();
    // TMatrixDSym cov;
    // fitter->GetCovarianceMatrix()
    // cov.Use(fitter->GetNumberTotalParameters(), fitter->GetCovarianceMatrix());
    // cov.Print();

    // backup new parameters
    params_vector backup_new(int2size_t(par_num));
    tfSum->GetParameters(backup_new.pars.data());
    double chi2_backup_new = hist->Chisquare(tfSum, "R");

    if (m_d->verbose_flag) { fmt::print("* new: {:g} --> chi2:  {:f} -- *", backup_new, chi2_backup_new); }

    if (chi2_backup_new > chi2_backup_old)
    {
        tfSum->SetParameters(backup_old.pars.data());
        new_sig_func->SetParameters(backup_old.pars.data());
        fmt::print("{}\n", "\t [ FAILED - restoring old params ]");
    }
    else
    {
        // fmt::print("\n\tIS-OK: {:g} vs. {:g} -> {:f}", tfSum->GetMaximum(), hist->GetMaximum(),
        //        hist->Chisquare(tfSum, "R"));

        if (m_d->verbose_flag) fmt::print("{}\n", "\t [ OK ]");
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
        tfSig->Clone(tools::format_name(hfp->get_name(), m_d->function_decorator + "_sig")));
    hist->GetListOfFunctions()->Add(
        tfBkg->Clone(tools::format_name(hfp->get_name(), m_d->function_decorator + "_bkg")));

    return true;
}

auto fitter::set_flags(priority_mode new_mode) -> void { m_d->mode = new_mode; }

auto fitter::set_default_parameters(fit_entry* defs) -> void { m_d->defpars = defs; }

auto fitter::set_replacement(const TString& src, const TString& dst) -> void
{
    m_d->rep_src = src;
    m_d->rep_dst = dst;
}

auto fitter::set_name_decorator(const TString& decorator) -> void { m_d->name_decorator = decorator; };
auto fitter::clear_name_decorator() -> void { m_d->name_decorator = "*"; };

auto fitter::set_function_decorator(const TString& decorator) -> void { m_d->function_decorator = decorator; };

auto fitter::set_draw_bits(bool sum, bool sig, bool bkg) -> void
{
    m_d->draw_sum = sum;
    m_d->draw_sig = sig;
    m_d->draw_bkg = bkg;
}

auto fitter::prop_sum() -> tools::draw_properties& { return m_d->prop_sum; }
auto fitter::prop_sig() -> tools::draw_properties& { return m_d->prop_sig; }
auto fitter::prop_bkg() -> tools::draw_properties& { return m_d->prop_bkg; }

auto fitter::print() const -> void
{
    for (auto it = m_d->hfpmap.begin(); it != m_d->hfpmap.end(); ++it)
    {
        it->second->print();
    }
}

auto fitter::clear() -> void { m_d->hfpmap.clear(); }

namespace tools
{
auto select_source(const char* filename, const char* auxname) -> source
{
#if __cplusplus >= 201703L
    const auto s_ref = std::filesystem::exists(filename);
    const auto s_aux = std::filesystem::exists(auxname);

    if (!s_ref and !s_aux) return source::none;
    if (s_ref and !s_aux) return source::only_reference;
    if (!s_ref and s_aux) return source::only_auxiliary;

    const std::filesystem::file_time_type mod_ref = std::filesystem::last_write_time(filename);
    const std::filesystem::file_time_type mod_aux = std::filesystem::last_write_time(auxname);
#else
    struct stat st_ref;
    struct stat st_aux;

    const auto s_ref = stat(filename, &st_ref) == 0;
    const auto s_aux = stat(auxname, &st_aux) == 0;

    if (!s_ref and !s_aux) { return source::none; }
    if (s_ref and !s_aux) { return source::only_reference; }
    if (!s_ref and s_aux) { return source::only_auxiliary; }

    const auto mod_ref = (long long)st_ref.st_mtim.tv_sec;
    const auto mod_aux = (long long)st_aux.st_mtim.tv_sec;
#endif

    return mod_aux > mod_ref ? source::auxiliary : source::reference;
}

auto draw_properties::apply_style(TF1* function) -> void
{
#if __cplusplus >= 201703L
    if (line_color) { function->SetLineColor(*line_color); }
    if (line_width) { function->SetLineWidth(*line_width); }
    if (line_style) { function->SetLineStyle(*line_style); }
#else
    if (line_color != -1) { function->SetLineColor(line_color); }
    if (line_width != -1) { function->SetLineWidth(line_width); }
    if (line_style != -1) { function->SetLineStyle(line_style); }
#endif
}

auto format_name(const TString& name, const TString& decorator) -> TString
{
    TString str = decorator;
    str.ReplaceAll("*", name);
    return str;
}

auto parse_line_entry(const TString& line, int /*version*/) -> std::unique_ptr<fit_entry>
{
    return parser::parse_line_entry_v1(line);
}

} // namespace tools

} // namespace hf

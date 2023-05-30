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

#include <fmt/core.h>
#include <fmt/ranges.h>

#include "hellofitty.hpp"

#include "parser.hpp"

#include <TF1.h>
#include <TH1.h>
#include <TList.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <sys/stat.h>
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
    CONSTEXPR auto parse(format_parse_context& ctx) -> format_parse_context::iterator
    {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e' || *it == 'g')) presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}') format_error("invalid format");

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

template <> struct fmt::formatter<hf::fit_entry>
{
    // Presentation format: 'f' - fixed, 'e' - exponential, 'g' - either.
    char presentation = 'g';

    // Parses format specifications of the form ['f' | 'e' | 'g'].
    CONSTEXPR auto parse(format_parse_context& ctx) -> format_parse_context::iterator
    {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e' || *it == 'g')) presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}') format_error("invalid format");

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

/// Structure stores a set of values for a single function parameters like the the mean value,
/// lwoer or upper boundaries, free or fixed fitting mode.
struct function_impl final
{
    TString body_string;
    TF1 function_obj;

    /// Accept param value and fit mode
    /// @param par_value initial parameter value
    /// @param par_mode parameter fitting mode, see @ref fit_mode
    explicit function_impl(TString body, Double_t range_min, Double_t range_max)
    {
        function_obj = TF1("", body, range_min, range_max, TF1::EAddToList::kNo);
        body_string = std::move(body);
    }

    auto print(bool detailed) const -> void
    {
        fmt::print("  Function: {}    params: {}\n", body_string.Data(), 0);

        if (detailed) { function_obj.Print("V"); }
    }
};

struct fit_entry_impl
{
    TString hist_name; // histogram name

    Double_t range_min; // function range
    Double_t range_max; // function range

    int rebin{0}; // rebin, 0 == no rebin
    bool fit_disabled{false};

    std::vector<function_impl> funcs;
    TString total_function_body;
    TF1 total_function_object;

    std::vector<param> pars;
    std::vector<Double_t> parameters_backup; // backup for parameters

    fit_entry_impl() : pars(10), parameters_backup(10) {}
    /// Does not recompile the total function. Use compile() after adding last function.
    auto add_function_lazy(TString formula) -> int
    {
        auto current_function_idx = funcs.size();
        funcs.emplace_back(formula, range_min, range_max);
        return size_t2int(current_function_idx);
    }

    auto compile() -> void
    {
        if (funcs.size() == 0) { return; }
        total_function_body =
            std::accumulate(std::next(funcs.begin()), funcs.end(), funcs[0].body_string,
                            [](TString a, hf::detail::function_impl b) { return std::move(a) + "+" + b.body_string; });

        total_function_object = TF1("", total_function_body, range_min, range_max, TF1::EAddToList::kNo);

        auto npars = int2size_t(total_function_object.GetNpar());
        pars.resize(npars);
        parameters_backup.resize(npars);
    }

    auto backup() -> void
    {
        parameters_backup.clear();
        for (auto& p : pars)
        {
            parameters_backup.push_back(p.value);
        }
    }

    auto restore() -> void
    {
        if (parameters_backup.size() != pars.size()) throw hf::length_error("Backup storage is empty.");

        const auto n = pars.size();
        for (std::remove_const<decltype(n)>::type i = 0; i < n; ++i)
        {
            pars[i].value = parameters_backup[i];
        }
    }
};

struct fitter_impl
{
    fitter::priority_mode mode;
    format_version input_format_version{format_version::detect};
    format_version output_format_version{format_version::v2};

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
} // namespace detail

fit_entry::fit_entry(TString hist_name, Double_t range_min, Double_t range_max)
    : m_d{make_unique<detail::fit_entry_impl>()}
{
    m_d->range_min = range_min;
    m_d->range_max = range_max;

    m_d->hist_name = std::move(hist_name);

    if (m_d->hist_name[0] == '@') { m_d->fit_disabled = true; }
}

fit_entry::~fit_entry() noexcept = default;

auto fit_entry::clone(TString new_name) const -> std::unique_ptr<fit_entry>
{
    auto cloned = make_unique<fit_entry>(std::move(new_name), m_d->range_min, m_d->range_max);
    for (const auto& func : m_d->funcs)
    {
        cloned->add_function(func.body_string);
    }
    return cloned;
}

auto fit_entry::clear() -> void { drop(); }

auto fit_entry::add_function(TString formula) -> int
{
    auto current_function_idx = m_d->add_function_lazy(std::move(formula));
    m_d->compile();
    return size_t2int(current_function_idx);
}

auto fit_entry::get_function(int fun_id) const -> const char*
{
    return m_d->funcs.at(int2size_t(fun_id)).body_string.Data();
}

auto fit_entry::init() -> void
{
    // for (size_t i = 0; i < m_d->pars.size(); ++i)
    //     if (m_d->pars[i].mode == fit_mode::fixed)
    //         m_d->function_sum.FixParameter(size_t2int(i), m_d->pars[i].value);
    //     else
    //     {
    //         m_d->function_sum.SetParameter(size_t2int(i), m_d->pars[i].value);
    //         if (m_d->pars[i].has_limits)
    //             m_d->function_sum.SetParLimits(size_t2int(i), m_d->pars[i].min, m_d->pars[i].max);
    //     } FIXME
}

auto fit_entry::set_param(int par_id, param value) -> void
{
    const auto upar_id = int2size_t(par_id);
    m_d->pars.at(upar_id) = value;
}

auto fit_entry::set_param(int par_id, Double_t val, param::fit_mode mode) -> void
{
    const auto upar_id = int2size_t(par_id);
    auto& par = m_d->pars.at(upar_id);

    par.value = val;
    par.min = 0;
    par.max = 0;
    par.mode = mode;
    par.has_limits = false;
}

auto fit_entry::set_param(int par_id, Double_t val, Double_t l, Double_t u, param::fit_mode mode) -> void
{
    const auto upar_id = int2size_t(par_id);
    auto& par = m_d->pars.at(upar_id);

    par.value = val;
    par.min = l;
    par.max = u;
    par.mode = mode;
    par.has_limits = true;
}

auto fit_entry::update_param(int par_id, Double_t value) -> void
{
    const auto upar_id = int2size_t(par_id);
    auto& par = m_d->pars.at(upar_id);

    par.value = value;
}

auto fit_entry::get_param(int par_id) const -> param
{
    const auto upar_id = int2size_t(par_id);
    return m_d->pars.at(upar_id);
}

auto fit_entry::get_param(int par_id) -> param&
{
    const auto upar_id = int2size_t(par_id);
    return m_d->pars.at(upar_id);
}

auto get_param_name_index(TF1* fun, const char* name) -> Int_t
{
    auto par_index = fun->GetParNumber(name);
    if (par_index == -1) { throw hf::index_error("No such parameter"); }
    return par_index;
}

auto fit_entry::get_param(const char* name) -> param&
{
    return get_param(get_param_name_index(&m_d->total_function_object, name));
}

auto fit_entry::get_param(const char* name) const -> param
{
    return get_param(get_param_name_index(&m_d->total_function_object, name));
}

auto fit_entry::get_name() const -> TString { return m_d->hist_name; }

auto fit_entry::get_fit_range_min() const -> Double_t { return m_d->range_min; }

auto fit_entry::get_fit_range_max() const -> Double_t { return m_d->range_max; }

auto fit_entry::get_functions_count() const -> int { return size_t2int(m_d->funcs.size()); }

auto fit_entry::get_function_object(int fun_id) const -> const TF1&
{
    return m_d->funcs.at(int2size_t(fun_id)).function_obj;
}

auto fit_entry::get_function_object(int fun_id) -> TF1&
{
    return const_cast<TF1&>(const_cast<const fit_entry*>(this)->get_function_object(fun_id));
}

auto fit_entry::get_function_object() const -> const TF1&
{
    // if (!m_d->total_function_object.IsValid()) { m_d->compile(); }
    m_d->compile();
    return m_d->total_function_object;
}

auto fit_entry::get_function_object() -> TF1&
{
    return const_cast<TF1&>(const_cast<const fit_entry*>(this)->get_function_object());
}

auto fit_entry::get_function_params_count() const -> int { return get_function_object().GetNpar(); }

auto fit_entry::get_flag_rebin() const -> int { return m_d->rebin; }

auto fit_entry::get_flag_disabled() const -> bool { return m_d->fit_disabled; }

auto fit_entry::print(bool detailed) const -> void
{
    fmt::print("## name: {:s}    rebin: {:d}   range: {:g} -- {:g}  param num: {:d}\n", m_d->hist_name.Data(),
               m_d->rebin, m_d->range_min, m_d->range_min, get_function_params_count());

    for (const auto& func : m_d->funcs)
    {
        func.print(detailed);
    }

    auto s = m_d->pars.size();
    for (decltype(s) i = 0; i < s; ++i)
    {
        fmt::print("   {}: ", i);
        m_d->pars[i].print();
    }

    // auto s = m_d->pars.size();
    // for (decltype(s) i = 0; i < s; ++i)
    // {
    //     fmt::print("   {}: ", i);
    //     m_d->pars[i].print();
    // }
    //
    // if (detailed)
    // {
    //     fmt::print("{}\n", "+++++++++ SIG function +++++++++");
    //     m_d->function_sig.Print("V");
    //     fmt::print("{}\n", "+++++++++ BKG function +++++++++");
    //     m_d->function_bkg.Print("V");
    //     fmt::print("{}\n", "+++++++++ SUM function +++++++++");
    //     m_d->function_sum.Print("V");
    //     fmt::print("{}\n", "++++++++++++++++++++++++++++++++");
    // } FIXME
}

auto fit_entry::backup() -> void { m_d->backup(); }

auto fit_entry::restore() -> void { m_d->restore(); }

auto fit_entry::drop() -> void { m_d->parameters_backup.clear(); }

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

    if (!filename) { fmt::print(stderr, "No reference input file given\n"); }
    if (!auxname) { fmt::print(stderr, "No output file given\n"); }

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
        insert_parameters(tools::parse_line_entry(line, m_d->input_format_version));
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
            fparfile << tools::format_line_entry(it->second.get(), m_d->output_format_version) << std::endl;
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

    TF1* tfSig = &hfp->get_function_object(0);
    TF1* tfBkg = &hfp->get_function_object(1);
    TF1* tfSum = &hfp->get_function_object();

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

    auto parnsig = tfSum->GetNpar();
    for (decltype(parnsig) i = 0; i < parnsig; ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        tfSig->SetParameter(i, par);
        tfSig->SetParError(i, err);

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

auto fitter::set_name_decorator(const TString& decorator) -> void { m_d->name_decorator = decorator; }
auto fitter::clear_name_decorator() -> void { m_d->name_decorator = "*"; }

auto fitter::set_function_decorator(const TString& decorator) -> void { m_d->function_decorator = decorator; }

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

auto HELLOFITTY_EXPORT detect_format(const TString& line) -> format_version
{
    if (line.First('|') == -1) { return hf::format_version::v1; }

    return hf::format_version::v2;
}

auto parse_line_entry(const TString& line, format_version version) -> std::unique_ptr<fit_entry>
{
    if (version == hf::format_version::detect) { version = tools::detect_format(line); }

    switch (version)
    {
        case format_version::v1:
            return parser::parse_line_entry_v1(line);
            break;
        case format_version::v2:
            return parser::parse_line_entry_v2(line);
            break;
        default:
            throw std::runtime_error("Parser not implemented");
            break;
    }
}

auto HELLOFITTY_EXPORT format_line_entry(const hf::fit_entry* entry, format_version version) -> TString
{
    switch (version)
    {
        case format_version::v1:
            return parser::format_line_entry_v1(entry);
            break;
        case format_version::v2:
            return parser::format_line_entry_v2(entry);
            break;
        default:
            throw std::runtime_error("Parser not implemented");
            break;
    }
}

} // namespace tools

} // namespace hf

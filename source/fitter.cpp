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

#include "details.hpp"
#include "parser.hpp"

#include <TF1.h>
#include <TH1.h>
#include <TList.h>

#include <fstream>

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <sys/stat.h>
#endif

bool hf::detail::fitter_impl::verbose_flag = true;

namespace
{
struct params_vector
{
    std::vector<double> pars;
    params_vector(size_t n) : pars(n) {}
};

enum class source
{
    none,
    only_reference,
    only_auxiliary,
    reference,
    auxiliary
};

auto select_source(const char* filename, const char* auxname = nullptr) -> source
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

auto apply_style(TF1* function, std::unordered_map<int, hf::draw_opts>& styles, int index) -> bool
{
    const auto style = styles.find(index);
    if (style != styles.cend())
    {
        style->second.apply(function);
        return true;
    }
    return false;
}

} // namespace

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <> struct fmt::formatter<params_vector>
{
    // Parses format specifications of the form ['f' | 'e' | 'g'].
    CONSTEXPR auto parse(format_parse_context& ctx) -> format_parse_context::iterator
    {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') FMT_THROW(format_error("invalid format"));

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Formats the point p using the parsed format specification (presentation)
    // stored in this formatter.
    auto format(const params_vector& p, format_context& ctx) const -> format_context::iterator
    {
        // ctx.out() is an output iterator to write to.

        for (const auto& par : p.pars)
            fmt::format_to(ctx.out(), "{:.g} ", par);

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
        if (it != end && *it != '}') FMT_THROW(format_error("invalid format"));

        // Return an iterator past the end of the parsed range:
        return it;
    }

    auto format(const hf::fit_entry& /*fitentry*/, format_context& ctx) const -> format_context::iterator
    {
        return ctx.out();
    }
};

namespace hf
{

auto fitter::set_verbose(bool verbose) -> void { detail::fitter_impl::verbose_flag = verbose; }

fitter::fitter() : m_d{make_unique<detail::fitter_impl>()} { m_d->mode = priority_mode::newer; }

fitter::fitter(fitter&&) = default;

auto fitter::operator=(fitter&&) -> fitter& = default;

fitter::~fitter() = default;

auto fitter::init_from_file(TString filename) -> bool
{
    m_d->par_ref = std::move(filename);

    if (!m_d->par_ref) { fmt::print(stderr, "No reference input file given\n"); }
    if (!m_d->par_aux) { fmt::print(stderr, "No output file given\n"); }

    auto selected = select_source(m_d->par_ref, m_d->par_aux);

    fmt::print("Available source: [{:c}] REF  [{:c}] AUX\n",
               selected != source::only_auxiliary and selected != source::none ? 'x' : ' ',
               selected != source::only_reference and selected != source::none ? 'x' : ' ');
    fmt::print("Selected source : [{:c}] REF  [{:c}] AUX\n",
               (selected == source::reference or selected == source::only_reference) ? 'x' : ' ',
               (selected == source::auxiliary or selected == source::only_auxiliary) ? 'x' : ' ');

    if (selected == source::none) return false;

    if (m_d->mode == priority_mode::reference)
    {
        if (selected == source::only_auxiliary)
            return false;
        else
            return import_parameters(m_d->par_ref);
    }

    if (m_d->mode == priority_mode::auxiliary)
    {
        if (selected == source::only_reference)
            return false;
        else
            return import_parameters(m_d->par_aux);
    }

    if (m_d->mode == priority_mode::newer)
    {
        if (selected == source::auxiliary or selected == source::only_auxiliary)
            return import_parameters(m_d->par_aux);
        else if (selected == source::reference or selected == source::only_reference)
            return import_parameters(m_d->par_ref);
    }

    return false;
}

auto fitter::init_from_file(TString filename, TString auxname, priority_mode mode) -> bool
{
    m_d->mode = mode;
    m_d->par_aux = std::move(auxname);
    return init_from_file(std::move(filename));
}

auto fitter::export_to_file(bool update_reference) -> bool
{
    if (!update_reference)
        return export_parameters(m_d->par_aux.Data());
    else
        return export_parameters(m_d->par_ref.Data());
}

auto fitter::insert_parameter(std::pair<TString, fit_entry> hfp) -> fit_entry*
{
    auto res = m_d->hfpmap.emplace(std::move(hfp));
    if (res.second) return &res.first->second;

    res.first->second = hfp.second;
    return &res.first->second;
}

auto fitter::insert_parameter(TString name, fit_entry hfp) -> fit_entry*
{
    return insert_parameter(std::make_pair(std::move(name), std::move(hfp)));
}

auto fitter::import_parameters(const TString& filename) -> bool
{
    std::ifstream fparfile(filename.Data());
    if (!fparfile.is_open())
    {
        fmt::print(stderr, "No file {} to open.\n", filename.Data());
        return false;
    }

    m_d->hfpmap.clear();

    std::string line;
    while (std::getline(fparfile, line))
    {
        insert_parameter(tools::parse_line_entry(line, m_d->input_format_version));
    }

    return true;
}

auto fitter::export_parameters(const TString& filename) -> bool
{
    std::ofstream fparfile(filename);
    if (!fparfile.is_open())
    {
        fmt::print(stderr, "Can't create AUX file {}. Skipping...\n", filename.Data());
        return false;
    }
    else
    {
        fmt::print("AUX file {} opened...  Exporting {} entries.\n", filename.Data(), m_d->hfpmap.size());
        for (auto it = m_d->hfpmap.begin(); it != m_d->hfpmap.end(); ++it)
        {
            fparfile << tools::format_line_entry(it->first, &it->second, m_d->output_format_version) << std::endl;
        }
    }
    return true;
}

auto fitter::find_fit(TH1* hist) const -> fit_entry* { return find_fit(hist->GetName()); }

auto fitter::find_fit(const char* name) const -> fit_entry*
{
    auto it = m_d->hfpmap.find(tools::format_name(name, m_d->name_decorator));
    if (it != m_d->hfpmap.end()) return &it->second;

    return nullptr;
}

auto fitter::fit(TH1* hist, const char* pars, const char* gpars) -> std::pair<bool, fit_entry*>
{
    fit_entry* hfp = find_fit(hist->GetName());
    if (!hfp)
    {
        fmt::print("HFP for histogram {:s} not found, trying from defaults.\n", hist->GetName());

        if (!m_d->generic_parameters.get_functions_count()) return {false, hfp};

        hfp = insert_parameter(TString(hist->GetName()), m_d->generic_parameters);
    }

    if (!hfp) return {false, hfp};

    hfp->backup();
    bool status = fit(hfp, hist, pars, gpars);

    if (!status) hfp->restore();

    return {status, hfp};
}

auto fitter::fit(fit_entry* hfp, TH1* hist, const char* pars, const char* gpars) -> bool
{
    Int_t bin_l = hist->FindBin(hfp->get_fit_range_min());
    Int_t bin_u = hist->FindBin(hfp->get_fit_range_max());

    hfp->m_d->prepare();

    if (hfp->get_flag_rebin() != 0)
    {
        // was_rebin = true;
        hist->Rebin(hfp->get_flag_rebin());
    }

    if (hist->Integral(bin_l, bin_u) == 0) return false;

    TF1* tfSum = &hfp->get_function_object();
    tfSum->SetName(tools::format_name(hist->GetName(), m_d->function_decorator));

    hist->GetListOfFunctions()->Clear();
    hist->GetListOfFunctions()->SetOwner(kTRUE);

    const auto par_num = tfSum->GetNpar();

    // backup old parameters
    params_vector backup_old(int2size_t(par_num));
    tfSum->GetParameters(backup_old.pars.data());
    double chi2_backup_old = hist->Chisquare(tfSum, "R");

    if (m_d->verbose_flag)
    {
        fmt::print("* old {} : {} --> chi2:  {:f} -- *\n", hist->GetName(), backup_old, chi2_backup_old);
    }

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

    if (m_d->verbose_flag)
    {
        fmt::print("* new {} : {} --> chi2:  {:f} -- *", hist->GetName(), backup_new, chi2_backup_new);
    }

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

    const auto functions_count = hfp->get_functions_count();

    for (auto i = 0; i < par_num; ++i)
    {
        double par = tfSum->GetParameter(i);
        double err = tfSum->GetParError(i);

        for (auto function = 0; function < functions_count; ++function)
        {
            auto& partial_function = hfp->get_function_object(function);
            if (i < partial_function.GetNpar())
            {
                partial_function.SetParameter(i, par);
                partial_function.SetParError(i, err);
            }
        }

        hfp->update_param(i, par);
    }

    auto complete_function = dynamic_cast<TF1*>(hist->GetListOfFunctions()->At(0));
    if (!apply_style(complete_function, hfp->m_d->partial_functions_styles, -1))
    {
        if (!apply_style(complete_function, m_d->partial_functions_styles, -1))
        {
            complete_function->ResetBit(TF1::kNotDraw);
        }
    }

    for (auto i = 0; i < functions_count; ++i)
    {
        auto& partial_function = hfp->get_function_object(i);
        // partial_function.SetName(tools::format_name(hfp->get_name(), m_d->function_decorator + "_function_" + i));

        auto cloned = dynamic_cast<TF1*>(
            partial_function.Clone(tools::format_name(hist->GetName(), m_d->function_decorator + "_function_" + i)));
        if (!apply_style(cloned, hfp->m_d->partial_functions_styles, i))
        {
            if (!apply_style(cloned, m_d->partial_functions_styles, i)) { cloned->ResetBit(TF1::kNotDraw); }
        }

        // tfSig->SetBit(TF1::kNotGlobal); TODO do I need it?

        hist->GetListOfFunctions()->Add(cloned);
    }

    return true;
}

auto fitter::set_generic_entry(fit_entry generic) -> void { m_d->generic_parameters = generic; }

auto fitter::has_generic_entry() -> bool { return m_d->generic_parameters.is_valid(); }

auto fitter::set_name_decorator(TString decorator) -> void { m_d->name_decorator = std::move(decorator); }
auto fitter::clear_name_decorator() -> void { m_d->name_decorator = "*"; }

auto fitter::set_function_decorator(TString decorator) -> void { m_d->function_decorator = std::move(decorator); }

auto fitter::set_function_style(int function_index) -> draw_opts&
{
    auto res = m_d->partial_functions_styles.insert({function_index, draw_opts()});
    if (res.second == true) return res.first->second;

    throw std::runtime_error("Function style already exists.");
}

auto fitter::set_function_style() -> draw_opts& { return set_function_style(-1); }

auto fitter::print() const -> void
{
    for (auto it = m_d->hfpmap.begin(); it != m_d->hfpmap.end(); ++it)
    {
        it->second.print(it->first);
    }
}

auto fitter::clear() -> void { m_d->hfpmap.clear(); }

} // namespace hf

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

#include "hellofitty.hpp"

#include "details.hpp"

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

fit_entry::fit_entry() : m_d{make_unique<detail::fit_entry_impl>()} {}

fit_entry::fit_entry(Double_t range_lower, Double_t range_upper) : m_d{make_unique<detail::fit_entry_impl>()}
{
    m_d->range_min = range_lower;
    m_d->range_max = range_upper;
}

fit_entry::fit_entry(const fit_entry& other) { m_d = make_unique<detail::fit_entry_impl>(*other.m_d); }

auto fit_entry::operator=(const fit_entry& other) -> fit_entry&
{
    m_d = make_unique<detail::fit_entry_impl>(*other.m_d);
    return *this;
}

fit_entry::~fit_entry() noexcept = default;

auto fit_entry::is_valid() const -> bool { return m_d->range_max > m_d->range_min; }

auto fit_entry::clear() -> void { drop(); }

auto fit_entry::add_function(TString formula) -> int
{
    auto current_function_idx = m_d->add_function_lazy(std::move(formula));
    m_d->compile();
    return current_function_idx;
}

auto fit_entry::get_function(int function_index) const -> const char*
{
    return m_d->funcs.at(int2size_t(function_index)).body_string.Data();
}

auto fit_entry::set_param(int par_id, param par) -> void
{
    const auto upar_id = int2size_t(par_id);
    m_d->pars.at(upar_id) = std::move(par);
}

auto fit_entry::set_param(int par_id, Double_t value, param::fit_mode mode) -> void
{
    set_param(par_id, param(value, mode));
}

auto fit_entry::set_param(int par_id, Double_t value, Double_t l, Double_t u, param::fit_mode mode) -> void
{
    set_param(par_id, param(value, l, u, mode));
}

auto fit_entry::update_param(int par_id, Double_t value) -> void
{
    const auto upar_id = int2size_t(par_id);
    auto& par = m_d->pars.at(upar_id);

    par.value = value;
}

auto fit_entry::get_param(int par_id) const -> const param&
{
    const auto upar_id = int2size_t(par_id);
    try
    {
        return m_d->pars.at(upar_id);
    }
    catch (std::out_of_range& e)
    {
        throw hf::index_error(e.what());
    }
}

auto fit_entry::get_param(int par_id) -> param&
{
    return const_cast<param&>(const_cast<const fit_entry*>(this)->get_param(par_id));
}

auto get_param_name_index(TF1* fun, const char* name) -> Int_t
{
    auto par_index = fun->GetParNumber(name);
    if (par_index == -1) { throw hf::index_error("No such parameter"); }
    return par_index;
}

auto fit_entry::get_param(const char* name) -> param&
{
    return get_param(get_param_name_index(&m_d->complete_function_object, name));
}

auto fit_entry::get_param(const char* name) const -> const param&
{
    return get_param(get_param_name_index(&m_d->complete_function_object, name));
}

auto fit_entry::get_fit_range_min() const -> Double_t { return m_d->range_min; }

auto fit_entry::get_fit_range_max() const -> Double_t { return m_d->range_max; }

auto fit_entry::get_functions_count() const -> int { return size_t2int(m_d->funcs.size()); }

auto fit_entry::get_function_object(int function_index) const -> const TF1&
{
    return m_d->funcs.at(int2size_t(function_index)).function_obj;
}

auto fit_entry::get_function_object(int function_index) -> TF1&
{
    return const_cast<TF1&>(const_cast<const fit_entry*>(this)->get_function_object(function_index));
}

auto fit_entry::get_function_object() const -> const TF1&
{
    if (!m_d->complete_function_object.IsValid()) { m_d->compile(); }
    return m_d->complete_function_object;
}

auto fit_entry::get_function_object() -> TF1&
{
    return const_cast<TF1&>(const_cast<const fit_entry*>(this)->get_function_object());
}

auto fit_entry::clone_function(int function_index, const char* new_name) -> std::unique_ptr<TF1>
{
    return std::unique_ptr<TF1>(dynamic_cast<TF1*>(get_function_object(function_index).Clone(new_name)));
}

auto fit_entry::clone_function(const char* new_name) -> std::unique_ptr<TF1>
{
    return std::unique_ptr<TF1>(dynamic_cast<TF1*>(get_function_object().Clone(new_name)));
}

auto fit_entry::get_function_params_count() const -> int { return get_function_object().GetNpar(); }

auto fit_entry::get_flag_rebin() const -> int { return m_d->rebin; }

auto fit_entry::get_flag_disabled() const -> bool { return m_d->fit_disabled; }

auto fit_entry::print(const TString& name, bool detailed) const -> void
{
    fmt::print("## name: {:s}    rebin: {:d}   range: {:g} -- {:g}  param num: {:d}  {:s}\n", name.Data(), m_d->rebin,
               m_d->range_min, m_d->range_min, get_function_params_count(), get_flag_disabled() ? "DISABLED" : "");

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

auto fit_entry::set_function_style(int function_index) -> draw_opts&
{
    auto res = m_d->partial_functions_styles.insert({function_index, draw_opts()});
    if (res.second == true) return res.first->second;

    throw std::runtime_error("Function style already exists.");
}

auto fit_entry::set_function_style() -> draw_opts& { return set_function_style(-1); }

} // namespace hf

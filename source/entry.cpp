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

template <> struct fmt::formatter<hf::entry>
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

    auto format(const hf::entry& /*fitentry*/, format_context& ctx) const -> format_context::iterator
    {
        return ctx.out();
    }
};

namespace hf
{

entry::entry() : m_d{make_unique<detail::entry_impl>()} {}

entry::entry(Double_t range_lower, Double_t range_upper) : m_d{make_unique<detail::entry_impl>()}
{
    m_d->range_min = range_lower;
    m_d->range_max = range_upper;
}

entry::entry(const entry& other) { m_d = make_unique<detail::entry_impl>(*other.m_d); }

auto entry::operator=(const entry& other) -> entry&
{
    m_d = make_unique<detail::entry_impl>(*other.m_d);
    return *this;
}

entry::~entry() noexcept = default;

auto entry::is_valid() const -> bool { return m_d->range_max > m_d->range_min; }

auto entry::clear() -> void { drop(); }

auto entry::add_function(std::string formula) -> int
{
    auto current_function_idx = m_d->add_function_lazy(std::move(formula));
    m_d->compile();
    return current_function_idx;
}

auto entry::get_function(int function_index) const -> const char*
{
    return m_d->funcs.at(int2size_t(function_index)).body_string.c_str();
}

auto entry::set_param(int par_id, hf::param par) -> void
{
    const auto upar_id = int2size_t(par_id);
    m_d->pars.at(upar_id) = std::move(par);
}

auto entry::set_param(int par_id, Double_t value, hf::param::fit_mode mode) -> void
{
    set_param(par_id, hf::param(value, mode));
}

auto entry::set_param(int par_id, Double_t value, Double_t l, Double_t u, param::fit_mode mode) -> void
{
    set_param(par_id, hf::param(value, l, u, mode));
}

auto entry::update_param(int par_id, Double_t value) -> void
{
    const auto upar_id = int2size_t(par_id);
    auto& par = m_d->pars.at(upar_id);

    par.value = value;
}

auto entry::get_param(int par_id) const -> hf::param { return param(par_id); }

auto get_param_name_index(TF1* fun, const char* name) -> Int_t
{
    auto par_index = fun->GetParNumber(name);
    if (par_index == -1) { throw hf::index_error("No such parameter"); }
    return par_index;
}

auto entry::get_param(const char* name) const -> hf::param
{
    return get_param(get_param_name_index(&m_d->complete_function_object, name));
}

auto entry::param(int par_id) const -> const hf::param&
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

auto entry::param(int par_id) -> hf::param&
{
    return const_cast<hf::param&>(const_cast<const entry*>(this)->param(par_id));
}

auto entry::param(const char* name) const -> const hf::param&
{
    return param(get_param_name_index(&m_d->complete_function_object, name));
}

auto entry::param(const char* name) -> hf::param&
{
    return const_cast<hf::param&>(const_cast<const entry*>(this)->param(name));
}

auto entry::set_fit_range(Double_t range_lower, Double_t range_upper) -> void
{
    m_d->range_min = range_lower;
    m_d->range_max = range_upper;
}

auto entry::get_fit_range_min() const -> Double_t { return m_d->range_min; }

auto entry::get_fit_range_max() const -> Double_t { return m_d->range_max; }

auto entry::get_functions_count() const -> int { return size_t2int(m_d->funcs.size()); }

auto entry::get_function_object(int function_index) const -> const TF1&
{
    return m_d->funcs.at(int2size_t(function_index)).function_obj;
}

auto entry::get_function_object(int function_index) -> TF1&
{
    return const_cast<TF1&>(const_cast<const entry*>(this)->get_function_object(function_index));
}

auto entry::get_function_object() const -> const TF1&
{
    if (!m_d->complete_function_object.IsValid()) { m_d->compile(); }
    return m_d->complete_function_object;
}

auto entry::get_function_object() -> TF1&
{
    return const_cast<TF1&>(const_cast<const entry*>(this)->get_function_object());
}

auto entry::clone_function(int function_index, const char* new_name) -> std::unique_ptr<TF1>
{
    return std::unique_ptr<TF1>(dynamic_cast<TF1*>(get_function_object(function_index).Clone(new_name)));
}

auto entry::clone_function(const char* new_name) -> std::unique_ptr<TF1>
{
    return std::unique_ptr<TF1>(dynamic_cast<TF1*>(get_function_object().Clone(new_name)));
}

auto entry::get_function_params_count() const -> int { return get_function_object().GetNpar(); }

auto entry::get_flag_rebin() const -> int { return m_d->rebin; }

auto entry::get_flag_disabled() const -> bool { return m_d->fit_disabled; }

auto entry::print(const std::string& name, bool detailed) const -> void
{
    fmt::print("## name: {:s}    rebin: {:d}   range: {:g} -- {:g}  param num: {:d}  {:s}\n", name, m_d->rebin,
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

auto entry::backup() -> void { m_d->backup(); }

auto entry::restore() -> void { m_d->restore(); }

auto entry::drop() -> void { m_d->parameters_backup.clear(); }

auto entry::set_function_style(int function_index) -> draw_opts&
{
    auto res = m_d->partial_functions_styles.insert({function_index, draw_opts()});
    if (res.second == true) return res.first->second;

    throw std::runtime_error("Function style already exists.");
}

auto entry::set_function_style() -> draw_opts& { return set_function_style(-1); }

} // namespace hf

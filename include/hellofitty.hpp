/*
    Fit'em All - a versatile histogram fitting tool for ROOT-based projects
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

#ifndef HELLOFITTY_HELLOFITTY_H
#define HELLOFITTY_HELLOFITTY_H

#include "HelloFitty/hellofitty_export.hpp"

#include <RtypesCore.h>
#include <TString.h>

#include <memory>
#include <optional>
#include <string>

#if __cplusplus < 201402L
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

class TF1;
class TH1;

namespace hf
{

namespace detail
{
struct fit_entry_impl;
struct fitter_impl;
} // namespace detail

namespace tools
{
struct draw_properties;
} // namespace tools

/// Structure stores a set of values for a single function parameters like the the mean value,
/// lwoer or upper boundaries, free or fixed fitting mode.
struct param final
{
    /// Fitting mode.
    enum class fit_mode
    {
        free, ///< parameter is free for fitting
        fixed ///< parameter is fixed
    };

    Double_t value{0.0};           ///< param value
    Double_t min{0.0};             ///< lower limit
    Double_t max{0.0};             ///< upper limit
    fit_mode mode{fit_mode::free}; ///< Parameter fitting mode
    bool has_limits{false};        ///< Remembers whether hit limits were set

    constexpr param() = default;

    /// Accept param value and fit mode
    /// @param par_value initial parameter value
    /// @param par_mode parameter fitting mode, see @ref fit_mode
    constexpr explicit param(Double_t par_value, param::fit_mode par_mode) : value(par_value), mode(par_mode) {}

    /// Accept param value, boundaries and fit mode
    /// @param par_value initial parameter value
    /// @param par_min value's lower fit boundary
    /// @param par_max value's upper fit boundary
    /// @param par_mode parameter fitting mode, see @ref fit_mode
    constexpr explicit param(Double_t par_value, Double_t par_min, Double_t par_max, param::fit_mode par_mode)
        : value(par_value), min(par_min), max(par_max), mode(par_mode), has_limits(true)
    {
    }

    /// Print param value line.
    auto print() const -> void;
};

/// Stores full description of a single fit entry - signal and background functions, and parameters.
class HELLOFITTY_EXPORT fit_entry final
{
public:
    constexpr fit_entry() = delete;
    explicit fit_entry(TString hist_name, TString formula_s, TString formula_b, Double_t range_lower,
                       Double_t range_upper);

    explicit fit_entry(const fit_entry&) = delete;
    auto operator=(const fit_entry&) -> fit_entry& = delete;

    fit_entry(fit_entry&&) = default;
    auto operator=(fit_entry&&) -> fit_entry& = default;

    ~fit_entry() noexcept;

    auto clone(TString new_name) const -> std::unique_ptr<fit_entry>;

    auto init() -> void;
    auto set_param(Int_t par, param value) -> void;
    auto set_param(Int_t par, Double_t val, param::fit_mode mode) -> void;
    auto set_param(Int_t par, Double_t val, Double_t min, Double_t max, param::fit_mode mode) -> void;
    auto update_param(Int_t par, Double_t val) -> void;

    auto get_name() const -> TString;

    auto get_param(Int_t par) const -> param;
    auto get_params_number() const -> Int_t;

    auto get_fit_range_min() const -> Double_t;
    auto get_fit_range_max() const -> Double_t;

    /// Return reference to signal function
    /// @return function reference
    auto get_sig_func() const -> const TF1&;

    /// Return reference to background function
    /// @return function reference
    auto get_bkg_func() const -> const TF1&;

    /// Return reference to signal+background function
    /// @return function reference
    auto get_sum_func() const -> const TF1&;

    /// Return reference to signal function
    /// @return function reference
    auto get_sig_func() -> TF1&;

    /// Return reference to background function
    /// @return function reference
    auto get_bkg_func() -> TF1&;

    /// Return reference to signal+background function
    /// @return function reference
    auto get_sum_func() -> TF1&;

    /// Return signal function string
    /// @return function string
    auto get_sig_string() const -> const TString&;

    /// Return background function string
    /// @return function string
    auto get_bkg_string() const -> const TString&;

    auto get_flag_rebin() const -> Int_t;
    auto get_flag_disabled() const -> bool;

    auto load(TF1* function) -> bool;

    auto clear() -> void;

    auto export_entry() const -> TString;

    /// Store current parameter in backup storage
    auto backup() -> void;
    /// Restore parameters from backup storage
    auto restore() -> void;
    /// Clear backup storage
    auto drop() -> void;

    auto print(bool detailed = false) const -> void;

private:
    std::unique_ptr<detail::fit_entry_impl> m_d;
};

class HELLOFITTY_EXPORT fitter final
{
public:
    enum class priority_mode
    {
        reference,
        auxiliary,
        newer
    };

    explicit fitter(priority_mode mode = priority_mode::newer);

    explicit fitter(const fitter&) = delete;
    auto operator=(const fitter&) -> fitter& = delete;

    fitter(fitter&&) = default;
    auto operator=(fitter&&) -> fitter& = default;

    ~fitter();

    auto clear() -> void;

    auto set_flags(priority_mode new_mode) -> void;
    auto set_default_parameters(fit_entry* defs) -> void;

    auto init_fitter_from_file(const char* filename, const char* auxname = nullptr) -> bool;
    auto export_fitter_to_file() -> bool;

    auto find_fit(TH1* hist) const -> fit_entry*;
    auto find_fit(const char* name) const -> fit_entry*;

    auto fit(TH1* hist, const char* pars = "BQ", const char* gpars = "") -> bool;
    auto fit(fit_entry* hfp, TH1* hist, const char* pars = "BQ", const char* gpars = "") -> bool;

    auto print() const -> void;

    static auto set_verbose(bool verbose) -> void;

    auto set_replacement(const TString& src, const TString& dst) -> void;

    auto insert_parameters(std::unique_ptr<fit_entry>&& hfp) -> void;
    auto insert_parameters(const TString& name, std::unique_ptr<fit_entry>&& hfp) -> void;

    auto set_name_decorator(const TString& decorator) -> void;
    auto clear_name_decorator() -> void;

    auto set_function_decorator(const TString& decorator) -> void;

    auto set_draw_bits(bool sum = true, bool sig = false, bool bkg = false) -> void;

    auto prop_sum() -> tools::draw_properties&;
    auto prop_sig() -> tools::draw_properties&;
    auto prop_bkg() -> tools::draw_properties&;

private:
    auto import_parameters(const std::string& filename) -> bool;
    auto export_parameters(const std::string& filename) -> bool;
    std::unique_ptr<detail::fitter_impl> m_d;
};

namespace tools
{
enum class source
{
    none,
    only_reference,
    only_auxiliary,
    reference,
    auxiliary
};

auto HELLOFITTY_EXPORT select_source(const char* filename, const char* auxname = nullptr) -> source;

struct draw_properties final
{
#if __cplusplus >= 201703L
    std::optional<Color_t> line_color;
    std::optional<Width_t> line_width;
    std::optional<Style_t> line_style;
#else
    Color_t line_color{-1};
    Width_t line_width{-1};
    Style_t line_style{-1};
#endif

    auto set_line_color(Int_t color) -> draw_properties&
    {
        line_color = color;
        return *this;
    }
    auto set_line_width(Int_t width) -> draw_properties&
    {
        line_width = width;
        return *this;
    }
    auto set_line_style(Int_t style) -> draw_properties&
    {
        line_style = style;
        return *this;
    }

    auto apply_style(TF1* function) -> void;
};

auto HELLOFITTY_EXPORT format_name(const TString& name, const TString& decorator) -> TString;

auto HELLOFITTY_EXPORT parse_line_entry(const TString& line, int version) -> std::unique_ptr<fit_entry>;

} // namespace tools

} // namespace hf

#ifdef FMT_RANGES_H_
template <> struct fmt::formatter<hf::param>
{
    char presentation = 'g';
    CONSTEXPR auto parse(format_parse_context& ctx) -> format_parse_context::iterator
    {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
        // if (it != end && (*it == 'f' || *it == 'e' || *it == 'g')) presentation = *it++;
        if (it != end && *it != '}') FMT_THROW(format_error("invalid format"));

        // Check if reached the end of the range:
        // if (it != end && *it != '}') format_error("invalid format");

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
            default:                                                // LCOV_EXCL_LINE
                throw std::runtime_error("Unknown hf::param mode"); // LCOV_EXCL_LINE
                break;                                              // LCOV_EXCL_LINE
        }

        if (par.mode == hf::param::fit_mode::free and par.has_limits == false)
        {
            fmt::format_to(ctx.out(), "{:g}", par.value);
        }
        else if (par.mode == hf::param::fit_mode::fixed and par.has_limits == false)
        {
            fmt::format_to(ctx.out(), "{:g} {:c}", par.value, sep);
        }
        else
        {
            fmt::format_to(ctx.out(), "{:g} {:c} {:g} {:g}", par.value, sep, par.min, par.max);

            return ctx.out();
        }

        return ctx.out();
    }
};
#endif

#endif // HELLOFITTY_HELLOFITTY_H

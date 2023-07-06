/*
    Hello Fitty - a versatile histogram fitting tool for ROOT-based projects
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
#include <stdexcept>
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

/// Exceptions

// Thrown when the entry line is ill-formed
class format_error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class index_error : public std::out_of_range
{
public:
    using std::out_of_range::out_of_range;
};

class length_error : public std::length_error
{
public:
    using std::length_error::length_error;
};

namespace detail
{
struct draw_opts_impl;
struct fit_entry_impl;
struct fitter_impl;
} // namespace detail

class HELLOFITTY_EXPORT draw_opts final
{
public:
    draw_opts();
    draw_opts(const draw_opts& other);

    /// Make function visible
    /// @param vis visibility
    /// @return reference to itself
    auto set_visible(bool vis) -> draw_opts&;
    /// Line color
    /// @param color color
    /// @return reference to itself
    auto set_line_color(Color_t color) -> draw_opts&;
    /// Line width
    /// @param width line width
    /// @return reference to itself
    auto set_line_width(Width_t width) -> draw_opts&;
    /// Line style
    /// @param style line style
    /// @return reference to itself
    auto set_line_style(Style_t style) -> draw_opts&;
    /// Apply style to function
    /// @param function function
    auto apply(TF1* function) const -> void;
    /// Print style info
    auto print() const -> void;

private:
    std::unique_ptr<detail::draw_opts_impl> m_d;
};

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

class fitter;

namespace parser
{
struct v1;
struct v2;
} // namespace parser

/// Stores full description of a single fit entry - signal and background functions, and parameters.
class HELLOFITTY_EXPORT fit_entry final
{
public:
    fit_entry();
    explicit fit_entry(Double_t range_lower, Double_t range_upper);

    fit_entry(const fit_entry& other);
    auto operator=(const fit_entry&) -> fit_entry&;

    explicit fit_entry(fit_entry&&) = default;
    auto operator=(fit_entry&&) -> fit_entry& = default;

    ~fit_entry() noexcept;

    auto init() -> void;

    /// Add function of given body to functions collection
    /// @param formula the function body
    /// @return function id
    auto add_function(TString formula) -> int;

    /// Get function body string
    /// @return function body as string
    /// @throw std::out_of_range if function_index incorrect
    auto get_function(int function_index) const -> const char*;

    auto set_param(int par_id, param par) -> void;
    auto set_param(int par_id, Double_t value, param::fit_mode mode) -> void;
    auto set_param(int par_id, Double_t value, Double_t min, Double_t max, param::fit_mode mode) -> void;
    auto update_param(int par_id, Double_t value) -> void;

    auto get_param(int par_id) -> param&;
    auto get_param(int par_id) const -> const param&;

    auto get_param(const char* name) -> param&;
    auto get_param(const char* name) const -> const param&;

    auto get_fit_range_min() const -> Double_t;
    auto get_fit_range_max() const -> Double_t;

    /// Return numbers of functions
    /// @return number of functions
    auto get_functions_count() const -> int;

    /// Return reference to given function
    /// @param function_index function id
    /// @return function reference
    /// @throw std::out_of_range
    auto get_function_object(int function_index) const -> const TF1&;

    /// Return reference to signal function
    /// @return function reference
    auto get_function_object(int function_index) -> TF1&;

    /// Return reference to total function
    /// @return function reference
    auto get_function_object() const -> const TF1&;

    /// Return reference to total function
    /// @return function reference
    auto get_function_object() -> TF1&;

    /// Clone function with new name. Function passes ownership of the clone.
    /// @param function_index function id
    /// @param new_name name for cloned function
    /// @return unique pointer to cloned function
    auto clone_function(int function_index, const char* new_name = nullptr) -> std::unique_ptr<TF1>;

    /// Clone total function with new name. Function passes ownership of the clone.
    /// @param new_name name for cloned function
    /// @return unique pointer to cloned function
    auto clone_function(const char* new_name = nullptr) -> std::unique_ptr<TF1>;

    /// Return numbers of params in total function.
    auto get_function_params_count() const -> int;

    auto get_flag_rebin() const -> Int_t;
    auto get_flag_disabled() const -> bool;

    auto clear() -> void;

    auto export_entry() const -> TString;

    /// Store current parameter in backup storage
    auto backup() -> void;
    /// Restore parameters from backup storage
    auto restore() -> void;
    /// Clear backup storage
    auto drop() -> void;

    auto set_function_style(int function_index) -> draw_opts&;
    auto set_function_style() -> draw_opts&;

    auto print(const TString& name, bool detailed = false) const -> void;

    friend hf::fitter;
    friend hf::parser::v1;
    friend hf::parser::v2;

private:
    std::unique_ptr<detail::fit_entry_impl> m_d;
};

/// Specifies the data file format
enum class format_version
{
    detect, ///< tries to detect the format, uses the same format for export
    v1,     ///< fixed two functions format
    v2,     ///< variable function number with params on the tail of line
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

    fitter();

    explicit fitter(const fitter&) = delete;
    auto operator=(const fitter&) -> fitter& = delete;

    fitter(fitter&&);
    auto operator=(fitter&&) -> fitter&;

    ~fitter();

    auto clear() -> void;

    /// For histograms which have no record in the entries collection, you can set a generic
    /// function and aparameters to be fit. It will not be used for disabled histograms.
    /// @param generic a generic histogram function object
    auto set_generic_entry(fit_entry generic) -> void;

    /// Load parameters from the reference input.
    /// @param input_file the input parameters
    /// @return the file was properly imported
    auto init_from_file(TString input_file) -> bool;
    /// Load parameters from the reference input or the auxiliary input, depend on mode.
    /// @param input_file the input file for parameters
    /// @param aux_file the output file for parameters
    /// @param mode source selection mode
    /// @return the file was properly imported
    auto init_from_file(TString input_file, TString aux_file, priority_mode mode = priority_mode::newer) -> bool;
    /// Force file exporting. If the output file was not set, the function does nothing.
    /// @return true if the file was written
    auto export_to_file(bool update_reference = false) -> bool;

    auto find_fit(TH1* hist) const -> fit_entry*;
    auto find_fit(const char* name) const -> fit_entry*;

    auto fit(TH1* hist, const char* pars = "BQ", const char* gpars = "") -> bool;
    auto fit(fit_entry* hfp, TH1* hist, const char* pars = "BQ", const char* gpars = "") -> bool;

    auto print() const -> void;

    static auto set_verbose(bool verbose) -> void;

    auto insert_parameter(TString name, fit_entry hfp) -> void;
    auto insert_parameter(std::pair<TString, fit_entry> hfp) -> void;

    auto set_name_decorator(TString decorator) -> void;
    auto clear_name_decorator() -> void;

    auto set_function_decorator(TString decorator) -> void;

    auto set_function_style(int function_index) -> draw_opts&;
    auto set_function_style() -> draw_opts&;

    auto get_function_style(int function_index) -> draw_opts&;
    auto get_function_style() -> draw_opts&;

private:
    auto import_parameters(const TString& filename) -> bool;
    auto export_parameters(const TString& filename) -> bool;
    std::unique_ptr<detail::fitter_impl> m_d;
};

namespace tools
{

auto HELLOFITTY_EXPORT format_name(const TString& name, const TString& decorator) -> TString;

/// Detect format of the line. A simple check of the pattern characteristic is made. In case of ill-formed line it may
/// result in false detection.
/// @param line entry line to be tested
/// @return format name
auto HELLOFITTY_EXPORT detect_format(const TString& line) -> format_version;

/// Parse the entry line according to given format, by default tries to detect the format.
/// @param line entry line to be parsed
/// @param version entry version
/// @return ownership of the parsed entry
auto HELLOFITTY_EXPORT parse_line_entry(const TString& line, format_version version = hf::format_version::detect)
    -> std::pair<TString, fit_entry>;

/// Export the entry to the text line using given format. By default the newest v2 is used.
/// @param entry fit entry
/// @param version entry version
/// @return the entry string
auto HELLOFITTY_EXPORT format_line_entry(const TString& name, const hf::fit_entry* entry,
                                         format_version version = hf::format_version::v2) -> TString;

} // namespace tools

} // namespace hf

#ifdef FMT_RANGES_H_
template <> struct fmt::formatter<hf::param>
{
    CONSTEXPR auto parse(format_parse_context& ctx) -> format_parse_context::iterator
    {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
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
#endif // FMT_RANGES_H_

#endif // HELLOFITTY_HELLOFITTY_H

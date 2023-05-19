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

#ifndef FITEMALL_FITEMALL_H
#define FITEMALL_FITEMALL_H

#include "fitemall/fitemall_export.hpp"

#include <memory>
#include <optional>
#include <string>

#include <RtypesCore.h>
#include <TString.h>

#if __cplusplus < 201402L
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

class TF1;
class TH1;

namespace fea
{

namespace detail
{
struct histogram_fit_impl;
struct fitter_impl;
} // namespace detail

namespace tools
{
struct draw_properties;
}

struct param final
{
    double value{0.0}; // value
    double lower{0.0}; // lower limit
    double upper{0.0}; // upper limit
    enum class fit_mode
    {
        free,
        fixed
    } mode{fit_mode::free};
    bool has_limits{false};

    constexpr param() = default;
    constexpr explicit param(double val, param::fit_mode m) : value(val), mode(m) {}
    constexpr explicit param(double val, double l, double u, param::fit_mode m)
        : value(val), lower(l), upper(u), mode(m), has_limits(true)
    {
    }
    void print() const;
};

class FITEMALL_EXPORT histogram_fit final
{
public:
    constexpr histogram_fit() = delete;
    explicit histogram_fit(TString hist_name, TString formula_s, TString formula_b,
                           Double_t range_lower, Double_t range_upper);

    explicit histogram_fit(histogram_fit&& other) = default;
    histogram_fit& operator=(histogram_fit&& other) = default;

    ~histogram_fit() noexcept;

    auto clone(TString new_name) const -> std::unique_ptr<histogram_fit>;

    auto init() -> void;
    auto set_param(Int_t par, param value) -> void;
    auto set_param(Int_t par, Double_t val, param::fit_mode mode) -> void;
    auto set_param(Int_t par, Double_t val, Double_t l, Double_t u, param::fit_mode mode) -> void;
    auto update_param(Int_t par, Double_t val) -> void;
    auto get_param(Int_t par) const -> param;
    auto get_params_number() const -> int;

    auto get_name() const -> TString;
    auto get_fit_range_l() const -> double;
    auto get_fit_range_u() const -> double;

    auto get_sig_func() const -> const TF1&;
    auto get_bkg_func() const -> const TF1&;
    auto get_sum_func() const -> const TF1&;

    auto get_sig_func() -> TF1&;
    auto get_bkg_func() -> TF1&;
    auto get_sum_func() -> TF1&;

    auto get_sig_string() const -> const TString&;
    auto get_bkg_string() const -> const TString&;

    auto get_flag_rebin() const -> int;
    auto get_flag_disabled() const -> bool;

    void print(bool detailed = false) const;
    bool load(TF1* f);

    void clear();

    TString export_entry() const;

    void save();
    void load();
    void drop();

private:
    histogram_fit(const histogram_fit& hfp) = delete;
    histogram_fit& operator=(const histogram_fit& hfp) = delete;

private:
    std::unique_ptr<detail::histogram_fit_impl> d;
};

class FITEMALL_EXPORT fitter final
{
public:
    enum class priority_mode
    {
        reference,
        auxiliary,
        newer
    };

    fitter(priority_mode mode = priority_mode::newer);
    ~fitter();

    void clear();

    void set_flags(priority_mode new_mode);
    void set_default_parameters(histogram_fit* defs);

    bool init_fitter_from_file(const char* filename, const char* auxname = 0);
    bool export_fitter_to_file();

    histogram_fit* find_fit(TH1* hist) const;
    histogram_fit* find_fit(const char* name) const;

    auto fit(TH1* hist, const char* pars = "B,Q", const char* gpars = "") -> bool;
    auto fit(histogram_fit* hfp, TH1* hist, const char* pars = "B,Q", const char* gpars = "")
        -> bool;

    void print() const;

    static auto set_verbose(bool verbose) -> void;

    auto set_replacement(const TString& src, const TString& dst) -> void;

    void insert_parameters(std::unique_ptr<histogram_fit>&& hfp);
    void insert_parameters(const TString& name, std::unique_ptr<histogram_fit>&& hfp);

    void set_name_decorator(const TString& decorator);
    void clear_name_decorator();

    void set_function_decorator(const TString& decorator);

    void set_draw_bits(bool sum = true, bool sig = false, bool bkg = false);

    auto prop_sum() -> tools::draw_properties&;
    auto prop_sig() -> tools::draw_properties&;
    auto prop_bkg() -> tools::draw_properties&;

private:
    bool import_parameters(const std::string& filename);
    bool export_parameters(const std::string& filename);
    std::unique_ptr<detail::fitter_impl> d;
};

namespace tools
{
enum class selected_source
{
    none,
    only_reference,
    only_auxiliary,
    reference,
    auxiliary
};

auto FITEMALL_EXPORT select_source(const char* filename, const char* auxname = 0)
    -> selected_source;

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

    draw_properties& set_line_color(Int_t color)
    {
        line_color = color;
        return *this;
    }
    draw_properties& set_line_width(Int_t width)
    {
        line_width = width;
        return *this;
    }
    draw_properties& set_line_style(Int_t style)
    {
        line_style = style;
        return *this;
    }

    void apply_style(TF1* f);
};

auto FITEMALL_EXPORT format_name(const TString& name, const TString& decorator) -> TString;

std::unique_ptr<histogram_fit> FITEMALL_EXPORT parse_line_entry(const TString& line, int version);

} // namespace tools

} // namespace fea
#endif // FITEMALL_FITEMALL_H

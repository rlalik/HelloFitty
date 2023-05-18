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

#ifndef FITTERFACTORY_H
#define FITTERFACTORY_H

#include "FitterFactory/FitterFactory_export.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <RtypesCore.h>
#include <TString.h>

#if __cplusplus < 201402L
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

class TF1;
class TH1;

namespace FF
{

struct HistogramFitImpl;
struct FitterFactoryImpl;

namespace Tools
{
struct DrawProperties;
}

struct Param final
{
    double value{0.0}; // value
    double lower{0.0}; // lower limit
    double upper{0.0}; // upper limit
    enum class FitMode
    {
        Free,
        Fixed
    } mode{FitMode::Free};
    bool has_limits{false};

    constexpr Param() = default;
    constexpr explicit Param(double val, Param::FitMode m) : value(val), mode(m) {}
    constexpr explicit Param(double val, double l, double u, Param::FitMode m)
        : value(val), lower(l), upper(u), mode(m), has_limits(true)
    {
    }
    void print() const;
};

class FITTERFACTORY_EXPORT HistogramFit final
{
public:
    constexpr HistogramFit() = delete;
    explicit HistogramFit(TString hist_name, TString formula_s, TString formula_b,
                          Double_t range_lower, Double_t range_upper);

    explicit HistogramFit(HistogramFit&& other) = default;
    HistogramFit& operator=(HistogramFit&& other) = default;

    ~HistogramFit() noexcept;

    auto clone(TString new_name) const -> std::unique_ptr<HistogramFit>;

    auto init() -> void;
    auto setParam(Int_t par, Param value) -> void;
    auto setParam(Int_t par, Double_t val, Param::FitMode mode) -> void;
    auto setParam(Int_t par, Double_t val, Double_t l, Double_t u, Param::FitMode mode) -> void;
    auto updateParam(Int_t par, Double_t val) -> void;
    auto getParam(Int_t par) const -> Param;
    auto getParamsNumber() const -> int;

    auto getName() const -> TString;
    auto getFitRangeL() const -> float;
    auto getFitRangeU() const -> float;

    auto getSigFunc() const -> const TF1&;
    auto getBkgFunc() const -> const TF1&;
    auto getSumFunc() const -> const TF1&;

    auto getSigFunc() -> TF1&;
    auto getBkgFunc() -> TF1&;
    auto getSumFunc() -> TF1&;

    auto getSigString() const -> const TString&;
    auto getBkgString() const -> const TString&;

    auto getFlagRebin() const -> int;
    auto getFlagDisabled() const -> bool;

    void print(bool detailed = false) const;
    bool load(TF1* f);

    void clear();

    TString exportEntry() const;

    void push();
    void pop();
    void apply();
    void drop();

private:
    HistogramFit(const HistogramFit& hfp) = delete;
    HistogramFit& operator=(const HistogramFit& hfp) = delete;

private:
    std::unique_ptr<HistogramFitImpl> d;
};

class FITTERFACTORY_EXPORT FitterFactory final
{
public:
    enum class PriorityMode
    {
        Reference,
        Auxiliary,
        Newer
    };

    FitterFactory(PriorityMode mode = PriorityMode::Newer);
    ~FitterFactory();

    void clear();

    void setFlags(PriorityMode new_mode);
    void setDefaultParameters(HistogramFit* defs);

    bool initFactoryFromFile(const char* filename, const char* auxname = 0);
    bool exportFactoryToFile();

    HistogramFit* findFit(TH1* hist) const;
    HistogramFit* findFit(const char* name) const;

    auto fit(TH1* hist, const char* pars = "B,Q", const char* gpars = "") -> bool;
    auto fit(HistogramFit* hfp, TH1* hist, const char* pars = "B,Q", const char* gpars = "")
        -> bool;

    void print() const;

    static auto setVerbose(bool verbose) -> void;

    auto setReplacement(const TString& src, const TString& dst) -> void;

    void insertParameters(std::unique_ptr<HistogramFit>&& hfp);
    void insertParameters(const TString& name, std::unique_ptr<HistogramFit>&& hfp);

    void setNameDecorator(const TString& decorator);
    void clearNameDecorator();

    void setFunctionDecorator(const TString& decorator);

    void setDrawBits(bool sum = true, bool sig = false, bool bkg = false);

    auto propSum() -> Tools::DrawProperties&;
    auto propSig() -> Tools::DrawProperties&;
    auto propBkg() -> Tools::DrawProperties&;

private:
    bool importParameters(const std::string& filename);
    bool exportParameters(const std::string& filename);
    std::unique_ptr<FitterFactoryImpl> d;
};

namespace Tools
{
enum class SelectedSource
{
    None,
    OnlyReference,
    OnlyAuxiliary,
    Reference,
    Auxiliary
};

auto FITTERFACTORY_EXPORT selectSource(const char* filename, const char* auxname = 0)
    -> SelectedSource;

struct DrawProperties final
{
#if __cplusplus >= 201703L
    std::optional<Int_t> line_color;
    std::optional<Int_t> line_width;
    std::optional<Int_t> line_style;
#else
    Int_t line_color{-1};
    Int_t line_width{-1};
    Int_t line_style{-1};
#endif

    DrawProperties& setLineColor(Int_t color)
    {
        line_color = color;
        return *this;
    }
    DrawProperties& setLineWidth(Int_t width)
    {
        line_width = width;
        return *this;
    }
    DrawProperties& setLineStyle(Int_t style)
    {
        line_style = style;
        return *this;
    }

    void applyStyle(TF1* f);
};

auto FITTERFACTORY_EXPORT format_name(const TString& name, const TString& decorator) -> TString;

std::unique_ptr<HistogramFit> FITTERFACTORY_EXPORT parseLineEntry(const TString& line, int version);

} // namespace Tools

} // namespace FF
#endif // FITTERFACTORY_H

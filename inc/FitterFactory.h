/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

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

#include <map>
#include <memory>
#include <string_view>

#include <TF1.h>
#include <TFormula.h>
#include <TString.h>

class TH1;

struct ParamValue
{
    double val{0}; // value
    double l{0};   // lower limit
    double u{0};   // upper limit
    enum class FitMode
    {
        Free,
        Fixed
    } mode{FitMode::Free};
    bool has_limits{false};

    ParamValue() = default;
    ParamValue(double val, ParamValue::FitMode mode) : val(val), mode(mode) {}
    ParamValue(double val, double l, double u, ParamValue::FitMode mode)
        : val(val), l(l), u(u), mode(mode), has_limits(true)
    {
    }
    void print();
};

using ParamVector = std::vector<ParamValue>;

class HistogramFitParams
{
public:
    TString hist_name;     // histogram name
    TString sig_string;    // signal and background functions
    TString bkg_string;

    Double_t range_l; // function range
    Double_t range_u; // function range

    int rebin{0}; // rebin, 0 == no rebin
    bool fit_disabled{false};

    ParamVector pars;
    TF1 function_sig;
    TF1 function_bkg;
    TF1 function_sum;

    HistogramFitParams() = delete;
    HistogramFitParams(const TString& hist_name, const TString& formula_s, const TString& formula_b,
                       Double_t range_lower, Double_t range_upper);
    HistogramFitParams(HistogramFitParams&& other) = default;
    HistogramFitParams& operator=(HistogramFitParams&& other) = default;

    ~HistogramFitParams() = default;

    void init(const TString& function_format_name);
    auto clone(const TString& new_name) const -> std::unique_ptr<HistogramFitParams>;
    void setParam(Int_t par, ParamValue value);
    void setParam(Int_t par, Double_t val, ParamValue::FitMode mode);
    void setParam(Int_t par, Double_t val, Double_t l, Double_t u, ParamValue::FitMode mode);
    void print(bool detailed = false) const;
    void printInline() const;
    bool update();
    bool update(TF1* f);

    void clear();

    TString exportEntry() const;

    static std::unique_ptr<HistogramFitParams> parseEntryFromFile(const TString& line);

    void push();
    void pop();
    void apply();
    void drop();

private:
    HistogramFitParams(const HistogramFitParams& hfp) = delete;
    HistogramFitParams& operator=(const HistogramFitParams& hfp) = delete;

private:
    std::vector<Double_t> backup_p; // backup for parameters
    std::vector<Double_t> backup_e; // backup for par errors
};

using HfpEntry = std::pair<TString, std::unique_ptr<HistogramFitParams>>;

class FitterFactory
{
public:
    enum class PriorityMode
    {
        Reference,
        Auxilary,
        Newer
    };

    FitterFactory(PriorityMode flags = PriorityMode::Newer);
    virtual ~FitterFactory();

    void clear();

    PriorityMode setFlags(PriorityMode new_mode);
    void setDefaultParameters(HistogramFitParams* defs);

    bool initFactoryFromFile(const char* filename, const char* auxname = 0);
    bool exportFactoryToFile();

    HistogramFitParams* findParams(TH1* hist) const;
    HistogramFitParams* findParams(const char* name) const;

    bool fit(TH1* hist, const char* pars = "B,Q", const char* gpars = "");
    bool fit(HistogramFitParams* hfp, TH1* hist, const char* pars = "B,Q", const char* gpars = "",
             double min_entries = 0);

    void print() const;

    static void setVerbose(bool verbose) { verbose_flag = verbose; }

    inline void setEntriesLowLimit(double min) { min_entries = min; }

    inline void setReplacement(const TString& src, const TString& dst)
    {
        rep_src = src;
        rep_dst = dst;
    }
    TString format_name(const TString& name, const TString& decorator) const;

    void insertParameters(std::unique_ptr<HistogramFitParams> hfp);
    void insertParameters(const TString& name, std::unique_ptr<HistogramFitParams> hfp);

    void setNameDecorator(const TString& decorator) { name_decorator = decorator; };
    void clearNameDecorator() { name_decorator = "*"; };

    void setFunctionDecorator(const TString& decorator) { function_decorator = decorator; };

private:
    bool import_parameters(std::string_view filename);
    bool export_parameters(std::string_view filename);

    PriorityMode mode;

    bool has_defaults;
    static bool verbose_flag;

    TString par_ref;
    TString par_aux;

    HistogramFitParams* defpars;
    std::map<TString, std::unique_ptr<HistogramFitParams>> hfpmap;

    double min_entries;

    TString name_decorator{"*"};
    TString function_decorator{"f_*"};

    TString rep_src;
    TString rep_dst;
};

#endif // FITTERFACTORY_H

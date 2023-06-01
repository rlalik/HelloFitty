#ifndef HELLOFITTY_DETAILS_H
#define HELLOFITTY_DETAILS_H

#include <TF1.h>
#include <TString.h>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <numeric>

namespace
{
/// Based on
/// https://stackoverflow.com/questions/27490762/how-can-i-convert-to-size-t-from-int-safely
constexpr auto size_t2int(size_t val) -> int
{
    return (val <= std::numeric_limits<int>::max()) ? static_cast<int>(val) : -1;
}
constexpr auto int2size_t(int val) -> size_t { return (val < 0) ? __SIZE_MAX__ : static_cast<size_t>(val); }

} // namespace

namespace hf::detail
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
        funcs.emplace_back(std::move(formula), range_min, range_max);
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

    auto prepare() -> void
    {
        auto params_number = int2size_t(total_function_object.GetNpar());
        for (size_t i = 0; i < params_number; ++i)
        {
            if (pars[i].mode == hf::param::fit_mode::fixed)
            {
                total_function_object.FixParameter(size_t2int(i), pars[i].value);
            }
            else
            {
                total_function_object.SetParameter(size_t2int(i), pars[i].value);
                if (pars[i].has_limits) { total_function_object.SetParLimits(size_t2int(i), pars[i].min, pars[i].max); }
            }
        }
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

} // namespace hf::detail

#endif /* HELLOFITTY_DETAILS_H*/

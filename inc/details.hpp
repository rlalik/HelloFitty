#ifndef HELLOFITTY_DETAILS_H
#define HELLOFITTY_DETAILS_H

#include <TF1.h>
#include <TString.h>

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <numeric>
#include <unordered_map>

#if __cplusplus < 201402L
template <typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
using std::make_unique;
#endif

namespace
{
/// Based on
/// https://stackoverflow.com/questions/27490762/how-can-i-convert-to-size-t-from-int-safely
constexpr auto size_t2int(size_t val) -> int
{
    return (val <= std::numeric_limits<int>::max()) ? static_cast<int>(val) : -1;
}
constexpr auto int2size_t(int val) -> size_t { return (val < 0) ? __SIZE_MAX__ : static_cast<size_t>(val); }

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

namespace hf::detail
{

struct draw_opts_impl final
{
#if __cplusplus >= 201703L
    std::optional<Int_t> visible;
    std::optional<Color_t> line_color;
    std::optional<Width_t> line_width;
    std::optional<Style_t> line_style;
#else
    Int_t visible{-1};
    Color_t line_color{-1};
    Width_t line_width{-1};
    Style_t line_style{-1};
#endif
};

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

struct entry_impl
{
    Double_t range_min; // function range mix
    Double_t range_max; // function range max

    int rebin{0}; // rebin, 0 == no rebin
    bool fit_disabled{false};

    std::vector<function_impl> funcs;
    TString complete_function_body;
    TF1 complete_function_object;

    std::vector<param> pars;
    std::vector<Double_t> parameters_backup; // backup for parameters

    std::unordered_map<int, draw_opts> partial_functions_styles;

    entry_impl() : pars(10), parameters_backup(10) {}
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
        complete_function_body =
            std::accumulate(std::next(funcs.begin()), funcs.end(), funcs[0].body_string,
                            [](TString a, hf::detail::function_impl b) { return std::move(a) + "+" + b.body_string; });

        complete_function_object = TF1("", complete_function_body, range_min, range_max, TF1::EAddToList::kNo);

        auto npars = int2size_t(complete_function_object.GetNpar());
        pars.resize(npars);
        parameters_backup.resize(npars);
    }

    auto prepare() -> void
    {
        auto params_number = int2size_t(complete_function_object.GetNpar());
        for (size_t i = 0; i < params_number; ++i)
        {
            if (pars[i].mode == hf::param::fit_mode::fixed)
            {
                complete_function_object.FixParameter(size_t2int(i), pars[i].value);
            }
            else
            {
                complete_function_object.SetParameter(size_t2int(i), pars[i].value);
                if (pars[i].has_limits)
                {
                    complete_function_object.SetParLimits(size_t2int(i), pars[i].min, pars[i].max);
                }
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
    fit_qa_checker checker{hf::chi2checker()};

    TString par_ref;
    TString par_aux;

    entry generic_parameters;
    std::map<TString, entry> hfpmap;

    TString name_decorator{"*"};
    TString function_decorator{"f_*"};

    std::unordered_map<int, draw_opts> partial_functions_styles;

    template <class T>
    auto generic_fit(entry* hfp, entry_impl* hfp_m_d, const char* name, T* dataobj, const char* pars,
                     const char* gpars) -> bool
    {
        hfp_m_d->prepare();

        TF1* tfSum = &hfp->get_function_object();
        tfSum->SetName(tools::format_name(name, function_decorator));

        dataobj->GetListOfFunctions()->Clear();
        dataobj->GetListOfFunctions()->SetOwner(kTRUE);

        const auto par_num = tfSum->GetNpar();

        // backup old parameters
        params_vector backup_old(int2size_t(par_num));
        for (int i = 0; i < par_num; ++i)
            backup_old[int2size_t(i)] = hfp->get_param(i);

        double chi2_backup_old = dataobj->Chisquare(tfSum, "R");

        auto fit_res = dataobj->Fit(tfSum, pars, gpars, hfp->get_fit_range_min(), hfp->get_fit_range_max());

        TF1* new_sig_func = dynamic_cast<TF1*>(dataobj->GetListOfFunctions()->At(0));

        // TVirtualFitter * fitter = TVirtualFitter::GetFitter();
        // TMatrixDSym cov;
        // fitter->GetCovarianceMatrix()
        // cov.Use(fitter->GetNumberTotalParameters(), fitter->GetCovarianceMatrix());
        // cov.Print();

        // backup new parameters
        params_vector backup_new = backup_old;
        for (int i = 0; i < par_num; ++i)
            backup_new[int2size_t(i)].value = tfSum->GetParameter(i);

        double chi2_backup_new = dataobj->Chisquare(tfSum, "R");

        auto qa_res = checker(backup_old, chi2_backup_old, backup_new, chi2_backup_new, fit_res);

        if (qa_res > 0)
        {
            if (verbose_flag)
            {
                fmt::print(fmt::fg(fmt::color::royal_blue), "* old  {} : {} --> chi2:  {:} -- *\n", name, backup_old,
                           chi2_backup_old);
                fmt::print(fmt::fg(fmt::color::lime_green), "* new  {} : {} --> chi2:  {:} -- *", name, backup_new,
                           chi2_backup_new);
                fmt::print("{}\n", "\t [ OK ]");
            }
        }
        else if (qa_res == 0)
        {
            if (verbose_flag)
            {
                fmt::print(fmt::fg(fmt::color::orange), "* fine {} : {} --> chi2:  {:} -- *", name, backup_old,
                           chi2_backup_new);
                fmt::print("{}\n", "\t [ pass ]");
            }
        }
        else
        {
            for (int i = 0; i < par_num; ++i)
            {
                tfSum->SetParameter(i, backup_old[int2size_t(i)].value);
                new_sig_func->SetParameter(i, backup_old[int2size_t(i)].value);
            }

            if (verbose_flag)
            {
                fmt::print(fmt::fg(fmt::color::royal_blue), "* old  {} : {} --> chi2:  {:} -- *\n", name, backup_old,
                           chi2_backup_old);
                fmt::print(fmt::fg(fmt::color::crimson), "* new  {} : {} --> chi2:  {:} -- *", name, backup_new,
                           chi2_backup_new);
                fmt::print("{}\n", "\t [ FAILED - restoring old params ]");
            }
        }

        tfSum->SetChisquare(dataobj->Chisquare(tfSum, "R"));

        new_sig_func->SetChisquare(dataobj->Chisquare(tfSum, "R"));

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

        auto complete_function = dynamic_cast<TF1*>(dataobj->GetListOfFunctions()->At(0));
        if (!apply_style(complete_function, hfp_m_d->partial_functions_styles, -1))
        {
            if (!apply_style(complete_function, partial_functions_styles, -1))
            {
                complete_function->ResetBit(TF1::kNotDraw);
            }
        }

        for (auto i = 0; i < functions_count; ++i)
        {
            auto& partial_function = hfp->get_function_object(i);
            // partial_function.SetName(tools::format_name(hfp->get_name(), function_decorator + "_function_" + i));

            auto cloned = dynamic_cast<TF1*>(
                partial_function.Clone(tools::format_name(name, function_decorator + "_function_" + i)));
            if (!apply_style(cloned, hfp_m_d->partial_functions_styles, i))
            {
                if (!apply_style(cloned, partial_functions_styles, i)) { cloned->ResetBit(TF1::kNotDraw); }
            }

            // tfSig->SetBit(TF1::kNotGlobal); TODO do I need it?

            dataobj->GetListOfFunctions()->Add(cloned);
        }

        return true;
    }
};

} // namespace hf::detail

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <> struct fmt::formatter<hf::params_vector>
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
    auto format(const hf::params_vector& p, format_context& ctx) const -> format_context::iterator
    {
        // ctx.out() is an output iterator to write to.

        for (const auto& par : p)
            fmt::format_to(ctx.out(), "{:.{}} ", par.value, par.print_precision);

        return ctx.out();
    }
};

#endif /* HELLOFITTY_DETAILS_H*/

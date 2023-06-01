#include "parser.hpp"

#include "hellofitty.hpp"

#include <RtypesCore.h>
#include <TF1.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <TString.h>

#include <memory>

namespace hf::parser
{
auto v2::parse_line_entry(const TString& line) -> std::unique_ptr<fit_entry>
{
    TString line_ = line;
    line_.ReplaceAll("\t", " ");
    auto arr = std::unique_ptr<TObjArray>(line_.Tokenize(" "));

    if (arr->GetEntries() < 5) { throw hf::format_error("Not enough parameters"); };

    // auto hfp = make_unique<fit_entry>(dynamic_cast<TObjString*>(arr->At(0))->String(),        // hist name
    //                                   dynamic_cast<TObjString*>(arr->At(1))->String(),        // func val
    //                                   dynamic_cast<TObjString*>(arr->At(2))->String(),        // func val
    //                                   dynamic_cast<TObjString*>(arr->At(4))->String().Atof(), // low range
    //                                   dynamic_cast<TObjString*>(arr->At(5))->String().Atof());

    auto hfp = make_unique<fit_entry>(dynamic_cast<TObjString*>(arr->At(0))->String(),        // hist name
                                      dynamic_cast<TObjString*>(arr->At(1))->String().Atof(), // low range
                                      dynamic_cast<TObjString*>(arr->At(2))->String().Atof());

    // auto rebin_value = dynamic_cast<TObjString*>(arr->At(3))->String().Atoi(); TODO
    // hfp->set_rebin_flag(rebin_value); // TODO implement this

    const auto all_tokens = arr->GetEntries();
    auto token_id = 4;

    for (; token_id < all_tokens; token_id++)
    {
        const auto token = dynamic_cast<TObjString*>(arr->At(token_id))->String();
        if (token == "|") { break; }

        if (token == ":" or token == "f" or token == "F") { throw hf::format_error("Param signature detected"); }

        hfp->m_d->add_function_lazy(token);
    }
    hfp->m_d->compile();

    Int_t step = 0;

    auto params_count = hfp->get_function_params_count();
    auto current_param = 0;

    for (int i = token_id + 1; i < all_tokens; i += step)
    {
        if (current_param > params_count) { throw hf::format_error("To many parameters"); }

        Double_t l_, u_;
        param::fit_mode flag_;
        bool has_limits_ = false;

        const TString val = dynamic_cast<TObjString*>(arr->At(i))->String();
        const TString nval =
            ((i + 1) < arr->GetEntries()) ? dynamic_cast<TObjString*>(arr->At(i + 1))->String() : TString();

        auto par_ = val.Atof();
        if (nval == ":")
        {
            l_ = (i + 2) < arr->GetEntries() ? dynamic_cast<TObjString*>(arr->At(i + 2))->String().Atof() : 0;
            u_ = (i + 2) < arr->GetEntries() ? dynamic_cast<TObjString*>(arr->At(i + 3))->String().Atof() : 0;
            step = 4;
            flag_ = param::fit_mode::free;
            has_limits_ = true;
        }
        else if (nval == "F")
        {
            l_ = (i + 2) < arr->GetEntries() ? dynamic_cast<TObjString*>(arr->At(i + 2))->String().Atof() : 0;
            u_ = (i + 2) < arr->GetEntries() ? dynamic_cast<TObjString*>(arr->At(i + 3))->String().Atof() : 0;
            step = 4;
            flag_ = param::fit_mode::fixed;
            has_limits_ = true;
        }
        else if (nval == "f")
        {
            l_ = 0;
            u_ = 0;
            step = 2;
            flag_ = param::fit_mode::fixed;
            has_limits_ = false;
        }
        else
        {
            l_ = 0;
            u_ = 0;
            step = 1;
            flag_ = param::fit_mode::free;
            has_limits_ = false;
        }

        try
        {
            if (has_limits_) { hfp->set_param(current_param, par_, l_, u_, flag_); }
            else { hfp->set_param(current_param, par_, flag_); }
        }
        catch (const std::out_of_range&)
        {
            throw hf::format_error("To many parameters");
        }

        current_param++;
    }

    return hfp;
}

auto v2::format_line_entry(const hf::fit_entry* hist_fit) -> TString
{
    auto out =
        TString::Format("%c%s\t%g %g %d", hist_fit->get_flag_disabled() ? '@' : ' ', hist_fit->get_name().Data(),
                        hist_fit->get_fit_range_min(), hist_fit->get_fit_range_max(), hist_fit->get_flag_rebin());
    const auto function_count = hist_fit->get_functions_count();

    for (auto function_counter = 0; function_counter < function_count; ++function_counter)
    {
        out += TString::Format(" %s", hist_fit->get_function(function_counter));
    }

    out += " |";

    auto max_params = hist_fit->get_function_params_count();
    for (auto param_counter = 0; param_counter < max_params; ++param_counter)
    {
        const auto param = hist_fit->get_param(param_counter);
        const TString val = TString::Format("%g", param.value);
        const TString min = TString::Format("%g", param.min);
        const TString max = TString::Format("%g", param.max);

        char sep{0};

        switch (param.mode)
        {
            case param::fit_mode::free:
                if (param.has_limits) { sep = ':'; }
                else { sep = ' '; }
                break;
            case param::fit_mode::fixed:
                if (param.has_limits) { sep = 'F'; }
                else { sep = 'f'; }
                break;
        }

        if (param.mode == param::fit_mode::free and param.has_limits == false)
        {
            out += TString::Format("  %s", val.Data());
        }
        else if (param.mode == param::fit_mode::fixed and param.has_limits == false)
        {
            out += TString::Format("  %s %c", val.Data(), sep);
        }
        else { out += TString::Format("  %s %c %s %s", val.Data(), sep, min.Data(), max.Data()); }
    }

    return out;
}
} // namespace hf::parser

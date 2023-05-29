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
auto parse_line_entry_v1(const TString& line) -> std::unique_ptr<fit_entry>
{
    TString line_ = line;
    line_.ReplaceAll("\t", " ");
    auto arr = std::unique_ptr<TObjArray>(line_.Tokenize(" "));

    if (arr->GetEntries() < 6)
    {
        // std::cerr << "Error parsing line:\n " << line << "\n";
        return nullptr;
    };

    // auto hfp = make_unique<fit_entry>(dynamic_cast<TObjString*>(arr->At(0))->String(),        // hist name
    //                                   dynamic_cast<TObjString*>(arr->At(1))->String(),        // func val
    //                                   dynamic_cast<TObjString*>(arr->At(2))->String(),        // func val
    //                                   dynamic_cast<TObjString*>(arr->At(4))->String().Atof(), // low range
    //                                   dynamic_cast<TObjString*>(arr->At(5))->String().Atof());

    auto hfp = make_unique<fit_entry>(dynamic_cast<TObjString*>(arr->At(0))->String(),        // hist name
                                      dynamic_cast<TObjString*>(arr->At(4))->String().Atof(), // low range
                                      dynamic_cast<TObjString*>(arr->At(5))->String().Atof());

    const auto sig_id = hfp->add_function(dynamic_cast<TObjString*>(arr->At(1))->String());
    const auto bkg_id = hfp->add_function(dynamic_cast<TObjString*>(arr->At(2))->String());

    std::array<int, 2> param_counter = {hfp->get_function_params_count(sig_id), hfp->get_function_params_count(bkg_id)};

    Int_t step = 0;

    auto entries = arr->GetEntries();

    size_t function_counter = 0;
    size_t current_param = 0;
    for (int i = 6; i < entries; i += step)
    {
        if (param_counter[function_counter] == 0)
        {
            function_counter++;
            current_param = 0;
        }

        if (function_counter > 1) { throw std::length_error("To many parameters"); }

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

        if (has_limits_) { hfp->set_param(function_counter, current_param, par_, l_, u_, flag_); }
        else { hfp->set_param(function_counter, current_param, par_, flag_); }

        current_param++;
        param_counter[function_counter]--;
    }

    return hfp;
}

auto format_line_entry_v1(const hf::fit_entry* hist_fit) -> TString
{
    auto out =
        TString::Format("%c%s\t%s %s %d %.0f %.0f", hist_fit->get_flag_disabled() ? '@' : ' ',
                        hist_fit->get_name().Data(), hist_fit->get_function(0), hist_fit->get_function(1),
                        hist_fit->get_flag_rebin(), hist_fit->get_fit_range_min(), hist_fit->get_fit_range_max());
    auto limit = hist_fit->get_function_params_count();

    for (size_t function_counter = 0; function_counter < 2; ++function_counter)
    {
        auto max_params = hist_fit->get_function_params_count(function_counter);
        for (size_t param_counter = 0; param_counter < max_params; ++param_counter)
        {
            const auto param = hist_fit->get_param(function_counter, param_counter);
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
                out += TString::Format(" %s", val.Data());
            }
            else if (param.mode == param::fit_mode::fixed and param.has_limits == false)
            {
                out += TString::Format(" %s %c", val.Data(), sep);
            }
            else { out += TString::Format(" %s %c %s %s", val.Data(), sep, min.Data(), max.Data()); }
        }
    }

    return out;
}

} // namespace hf::parser

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
auto parse_line_entry_v1(const TString& line) -> std::unique_ptr<histogram_fit>
{
    TString line_ = line;
    line_.ReplaceAll("\t", " ");
    TObjArray* arr = line_.Tokenize(" ");

    if (arr->GetEntries() < 6)
    {
        // std::cerr << "Error parsing line:\n " << line << "\n";
        delete arr;
        return nullptr;
    };

    auto hfp =
        std::make_unique<histogram_fit>(((TObjString*)arr->At(0))->String(),        // hist name
                                        ((TObjString*)arr->At(1))->String(),        // func val
                                        ((TObjString*)arr->At(2))->String(),        // func val
                                        ((TObjString*)arr->At(4))->String().Atof(), // low range
                                        ((TObjString*)arr->At(5))->String().Atof());

    auto npars = hfp->get_sum_func().GetNpar();

    Double_t par_, l_, u_;
    Int_t step = 0;
    Int_t parnum = 0;
    param::fit_mode flag_;
    bool has_limits_ = false;

    auto entries = arr->GetEntries();
    for (int i = 6; i < entries; i += step, ++parnum)
    {
        if (parnum >= npars)
        {
            delete arr;
            return nullptr;
        }

        TString val = ((TObjString*)arr->At(i))->String();
        TString nval =
            ((i + 1) < arr->GetEntries()) ? ((TObjString*)arr->At(i + 1))->String() : TString();

        par_ = val.Atof();
        if (nval == ":")
        {
            l_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 2))->String().Atof() : 0;
            u_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 3))->String().Atof() : 0;
            step = 4;
            flag_ = param::fit_mode::free;
            has_limits_ = true;
        }
        else if (nval == "F")
        {
            l_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 2))->String().Atof() : 0;
            u_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 3))->String().Atof() : 0;
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

        if (has_limits_)
            hfp->set_param(parnum, par_, l_, u_, flag_);
        else
            hfp->set_param(parnum, par_, flag_);
    }

    delete arr;

    return hfp;
}

auto format_line_entry_v1(const hf::histogram_fit* hist_fit) -> TString
{
    TString out = hist_fit->get_flag_disabled() ? "@" : " ";

    char sep;

    out = TString::Format("%s%s\t%s %s %d %.0f %.0f", out.Data(), hist_fit->get_name().Data(),
                          hist_fit->get_sig_string().Data(), hist_fit->get_bkg_string().Data(),
                          hist_fit->get_flag_rebin(), hist_fit->get_fit_range_l(),
                          hist_fit->get_fit_range_u());
    auto limit = hist_fit->get_params_number();

    for (decltype(limit) i = 0; i < limit; ++i)
    {
        TString v = TString::Format("%g", hist_fit->get_param(i).value);
        TString l = TString::Format("%g", hist_fit->get_param(i).lower);
        TString u = TString::Format("%g", hist_fit->get_param(i).upper);

        switch (hist_fit->get_param(i).mode)
        {
            case param::fit_mode::free:
                if (hist_fit->get_param(i).has_limits)
                    sep = ':';
                else
                    sep = ' ';
                break;
            case param::fit_mode::fixed:
                if (hist_fit->get_param(i).has_limits)
                    sep = 'F';
                else
                    sep = 'f';
                break;
        }

        if (hist_fit->get_param(i).mode == param::fit_mode::free and
            hist_fit->get_param(i).has_limits == 0)
            out += TString::Format(" %s", v.Data());
        else if (hist_fit->get_param(i).mode == param::fit_mode::fixed and
                 hist_fit->get_param(i).has_limits == 0)
            out += TString::Format(" %s %c", v.Data(), sep);
        else
            out += TString::Format(" %s %c %s %s", v.Data(), sep, l.Data(), u.Data());
    }

    return out;
}

} // namespace hf::parser

#include "parser.hpp"

#include "FitterFactory.h"

#include <TF1.h>
#include <TObjArray.h>
#include <TObjString.h>

#include <memory>

namespace FF::Tools
{
auto parseLineEntry_v1(const TString& line) -> std::unique_ptr<HistogramFit>
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
        std::make_unique<HistogramFit>(((TObjString*)arr->At(0))->String(),        // hist name
                                       ((TObjString*)arr->At(1))->String(),        // func val
                                       ((TObjString*)arr->At(2))->String(),        // func val
                                       ((TObjString*)arr->At(4))->String().Atof(), // low range
                                       ((TObjString*)arr->At(5))->String().Atof());

    auto npars = hfp->getSumFunc().GetNpar();

    Double_t par_, l_, u_;
    Int_t step = 0;
    Int_t parnum = 0;
    Param::FitMode flag_;
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
            flag_ = Param::FitMode::Free;
            has_limits_ = true;
        }
        else if (nval == "F")
        {
            l_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 2))->String().Atof() : 0;
            u_ = (i + 2) < arr->GetEntries() ? ((TObjString*)arr->At(i + 3))->String().Atof() : 0;
            step = 4;
            flag_ = Param::FitMode::Fixed;
            has_limits_ = true;
        }
        else if (nval == "f")
        {
            l_ = 0;
            u_ = 0;
            step = 2;
            flag_ = Param::FitMode::Fixed;
            has_limits_ = false;
        }
        else
        {
            l_ = 0;
            u_ = 0;
            step = 1;
            flag_ = Param::FitMode::Free;
            has_limits_ = false;
        }

        if (has_limits_)
            hfp->setParam(parnum, par_, l_, u_, flag_);
        else
            hfp->setParam(parnum, par_, flag_);
    }

    delete arr;

    return hfp;
}

} // namespace FF::Tools

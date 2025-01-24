#include "hellofitty.hpp"

#include "hellofitty_config.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TMath.h>
#include <TROOT.h>

#include <fmt/core.h>
#include <fmt/printf.h>

#ifdef HAS_ROOTTOOLS
#    include <RootTools.h>
#endif

auto main(int argc, char* argv[]) -> int
{
    auto input_root = std::string(examples_bin_path) + "test_hist_output.root";
    auto input_param = std::string(examples_bin_path) + "test_output1.txt";

    if (argc < 3)
    {
        fmt::print(stderr, "Usage: {} file.root file_with_params\n", argv[0]);
        fmt::print(stderr, "Using defaults: {} {}\n", input_root, input_param);
    }
    else
    {
        input_root = argv[1];
        input_param = argv[2];
    }

    // load custom functions definitions
#ifdef HAS_ROOTTOOLS
    RootTools::MyMath();
#endif

    // create params output filename
    auto opf = fmt::sprintf("%s%s", input_param, ".out");

    // create fitting factory
    hf::fitter ff;
    ff.init_from_file(input_param.c_str(), opf.c_str());

    // uncomment this to print all entries
    // ff.print();

    // open root file
    TFile* file = TFile::Open(input_root.c_str(), "READ");

    TFile* ofile = nullptr;
    if (argc >= 4)
    {
        ofile = TFile::Open(argv[3], "RECREATE");
        ofile->cd();
    }

    // do fitting for each TH1 object found
    TKey* key = nullptr;
    TIter nextkey(file->GetListOfKeys());
    while ((key = dynamic_cast<TKey*>(nextkey())))
    {
        const char* classname = key->GetClassName();
        TClass* cl = gROOT->GetClass(classname);
        if (!cl) { continue; }

        TObject* obj = key->ReadObj();
        fmt::print("*** {:s}\n", obj->GetName());
        if (obj->InheritsFrom("TH1"))
        {
            TH1* h = dynamic_cast<TH1*>(obj->Clone(obj->GetName()));
            ff.fit(h, "BQ0");
            if (ofile) { h->Write(); }
        }
    }

    ff.export_to_file();

    file->Close();

    if (ofile)
    {
        ofile->Write();
        ofile->Close();
    }

    return 0;
}

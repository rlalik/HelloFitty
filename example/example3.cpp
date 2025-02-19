#include "hellofitty.hpp"

#include "hellofitty_config.h"

#include "example1.hpp"

auto main() -> int
{
    auto root_file_name = std::string(examples_bin_path) + "test_hist_input_3.root";
    auto hist = std::unique_ptr<TH1I>(create_root_file(root_file_name));
    auto root_outout_name = std::string(examples_bin_path) + "test_hist_output_3.root";

    auto input_name = std::string(examples_bin_path) + "test_input.txt";
    create_input_file(input_name);

    auto output1_name = std::string(examples_bin_path) + "test_output_3.txt";

    hf::fitter ff;
    ff.set_verbose(true);

    ff.set_function_style(0).set_line_color(1).set_line_width(1).set_line_style(2).set_visible(false).print();
    ff.set_function_style(1).set_line_color(1).set_line_width(2).set_line_style(9).print();
    ff.set_function_style().set_visible(true).print();

    /** First usage using HFP object **/
    fmt::print("{}", "\n ---- FIRST USAGE ---\n\n");
    ff.init_from_file(input_name, output1_name);

    auto hfp = ff.find_fit("test_hist");
    hfp->set_function_style(0).set_visible(true);
    if (hfp)
    {
        hfp->backup();
        if (!ff.fit(hist.get(), hfp, "BQ0", "").first) { hfp->restore(); }
    }
    else { fmt::print(stderr, "{}\n", "No function found"); }

    hfp->print("test_hist");

    // Let's make a second fit on the same histogram without removing the previous one
    auto single_peak = hf::entry(1.5, 4.5);
    single_peak.add_function("gaus");
    single_peak.set_param(0, hfp->get_param(0).value);
    single_peak.set_param(1, hfp->get_param(1).value);
    single_peak.set_param(2, hfp->get_param(2).value);
    single_peak.print("single_peak_generic");

    auto hfp_single_peak = ff.find_or_make("test_hist_peak", &single_peak);
    hfp_single_peak->print("test_hist_peak");

    hfp_single_peak->set_function_style().set_line_color(kBlue);

    // This will add new blue fit
    ff.fit(hfp_single_peak, hist.get(), "N");
    hfp_single_peak->print("test_hist_peak");

    // Let's change fit range and refit, the previous blue fit should be removed
    hfp_single_peak->set_fit_range(2, 4);
    hfp_single_peak->set_function_style().set_line_color(kGreen);

    ff.fit(hfp_single_peak, hist.get(), "+");
    hfp_single_peak->print("test_hist_peak");

    ff.export_to_file();

    auto list = hist->GetListOfFunctions();
    for (int i = 0; i < list->GetEntries(); ++i)
    {
        fmt::print("\n-- Function #{}\n\n", i + 1);
        list->At(i)->Print();
    }

    TFile* fp = TFile::Open(root_outout_name.c_str(), "RECREATE");
    if (fp) { hist->Write(); }
    else
    {
        fmt::print(stderr, "File {:s} not open\n", root_outout_name);
        abort();
    }
    fp->Close();
    delete fp;

    return 0;
}

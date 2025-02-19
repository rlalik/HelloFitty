#include "hellofitty.hpp"

#include "hellofitty_config.h"

#include "example1.hpp"

auto main() -> int
{
    auto root_file_name = std::string(examples_bin_path) + "test_hist_input_1c.root";
    auto hist = std::unique_ptr<TH1I>(create_root_file(root_file_name));
    auto root_outout_name = std::string(examples_bin_path) + "test_hist_output_1c.root";

    auto input_name = std::string(examples_bin_path) + "test_input.txt";
    create_input_file(input_name);

    auto output3_name = std::string(examples_bin_path) + "test_output_1c.txt";

    hf::fitter ff;
    ff.set_verbose(true);

    ff.set_function_style(0).set_line_color(1).set_line_width(1).set_line_style(2).set_visible(false).print();
    ff.set_function_style(1).set_line_color(1).set_line_width(2).set_line_style(9).print();
    ff.set_function_style().set_visible(true).print();

    /** Third usage using histogram object **/
    fmt::print("{}", "\n ---- THIRD USAGE ---\n\n");
    ff.init_from_file(input_name, output3_name, hf::fitter::priority_mode::reference);

    fmt::print("{}", "\nBefore fitting:\n");
    ff.print();

    if (!ff.fit(hist.get(), "BQ0").first) { fmt::print(stderr, "{}\n", "No function found"); }

    fmt::print("{}", "\nAfter fitting:\n");
    ff.print();

    ff.export_to_file();

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

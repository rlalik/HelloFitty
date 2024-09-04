#include <gtest/gtest.h>

#include "hellofitty.hpp"
#include "parser.hpp"

TEST(TestsParserV1, ParsingLine)
{
    auto hfp0 = hf::tools::parse_line_entry("hist_1 gaus(0) 0  0  1 10  1  2 : 1 3  3 F 2 5", hf::format_version::v1);

    ASSERT_THROW(
        hf::tools::parse_line_entry("hist_1 gaus(0) 0  0  1 10  1  2 : 1 3  3 F 2 5  4 f", hf::format_version::v1),
        hf::format_error);

    ASSERT_THROW(hf::tools::parse_line_entry("hist_1 gaus(0) gaus(0)  0  1 10  1  2 : 1 3  3 F 2 5  4 f",
                                             hf::format_version::v1),
                 hf::format_error);

    ASSERT_THROW(hf::tools::parse_line_entry("hist_1 gaus(0) expo(0)  0  1 10  1  2 : 1 3  3 F 2 5  4 f",
                                             hf::format_version::v1),
                 hf::format_error);

    auto hfp1 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  0  1 10  1  2 : 1 3  3 F 2 5  4 f",
                                            hf::format_version::v1);

    ASSERT_STREQ(hfp1.second.get_function(0), "gaus(0)");
    ASSERT_STREQ(hfp1.second.get_function(1), "pol0(3)");

    ASSERT_EQ(hfp1.second.get_fit_range_min(), 1);
    ASSERT_EQ(hfp1.second.get_fit_range_max(), 10);

    ASSERT_EQ(hfp1.second.get_flag_rebin(), 0);
    ASSERT_EQ(hfp1.second.get_flag_disabled(), false);

    ASSERT_EQ(hfp1.second.get_function_params_count(), 4);

    ASSERT_EQ(hfp1.second.param(0).value, 1);
    ASSERT_EQ(hfp1.second.param(0).min, 0);
    ASSERT_EQ(hfp1.second.param(0).max, 0);
    ASSERT_EQ(hfp1.second.param(0).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp1.second.param(0).has_limits, false);

    ASSERT_EQ(hfp1.second.param(1).value, 2);
    ASSERT_EQ(hfp1.second.param(1).min, 1);
    ASSERT_EQ(hfp1.second.param(1).max, 3);
    ASSERT_EQ(hfp1.second.param(1).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp1.second.param(1).has_limits, true);

    ASSERT_EQ(hfp1.second.param(2).value, 3);
    ASSERT_EQ(hfp1.second.param(2).min, 2);
    ASSERT_EQ(hfp1.second.param(2).max, 5);
    ASSERT_EQ(hfp1.second.param(2).mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(hfp1.second.param(2).has_limits, true);

    ASSERT_EQ(hfp1.second.param(3).value, 4);
    ASSERT_EQ(hfp1.second.param(3).min, 0);
    ASSERT_EQ(hfp1.second.param(3).max, 0);
    ASSERT_EQ(hfp1.second.param(3).mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(hfp1.second.param(3).has_limits, false);

    hfp1.second.print(hfp1.first);

    auto export1 = hf::tools::format_line_entry(hfp1.first, &hfp1.second, hf::format_version::v1);
    ASSERT_STREQ(export1, " hist_1\tgaus(0) pol0(3) 0 1 10  1  2 : 1 3  3 F 2 5  4 f");

    auto hfp2 = hf::tools::parse_line_entry("hist_2 gaus(0) pol0(3)  1  1 10  1  2 : 1 3  3 F 2 5  4 f",
                                            hf::format_version::v1);

    ASSERT_STREQ(hfp2.second.get_function(0), "gaus(0)");
    ASSERT_STREQ(hfp2.second.get_function(1), "pol0(3)");

    ASSERT_EQ(hfp2.second.get_fit_range_min(), 1);
    ASSERT_EQ(hfp2.second.get_fit_range_max(), 10);

    ASSERT_EQ(hfp2.second.get_flag_rebin(), 0);
    ASSERT_EQ(hfp2.second.get_flag_disabled(), false);

    ASSERT_EQ(hfp2.second.get_function_params_count(), 4);

    ASSERT_EQ(hfp2.second.param(0).value, 1);
    ASSERT_EQ(hfp2.second.param(0).min, 0);
    ASSERT_EQ(hfp2.second.param(0).max, 0);
    ASSERT_EQ(hfp2.second.param(0).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp2.second.param(0).has_limits, false);

    ASSERT_EQ(hfp2.second.param(1).value, 2);
    ASSERT_EQ(hfp2.second.param(1).min, 1);
    ASSERT_EQ(hfp2.second.param(1).max, 3);
    ASSERT_EQ(hfp2.second.param(1).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp2.second.param(1).has_limits, true);

    ASSERT_EQ(hfp2.second.param(2).value, 3);
    ASSERT_EQ(hfp2.second.param(2).min, 2);
    ASSERT_EQ(hfp2.second.param(2).max, 5);
    ASSERT_EQ(hfp2.second.param(2).mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(hfp2.second.param(2).has_limits, true);

    ASSERT_EQ(hfp2.second.param(3).value, 4);
    ASSERT_EQ(hfp2.second.param(3).min, 0);
    ASSERT_EQ(hfp2.second.param(3).max, 0);
    ASSERT_EQ(hfp2.second.param(3).mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(hfp2.second.param(3).has_limits, false);

    hfp2.second.print(hfp2.first);

    auto export2 = hf::parser::v1::format_line_entry(hfp2.first, &hfp2.second);
    ASSERT_STREQ(export2, " hist_2\tgaus(0) pol0(3) 0 1 10  1  2 : 1 3  3 F 2 5  4 f");

    auto hfp3 = hf::tools::parse_line_entry("hist_3 gaus(0) pol0(3)  1  1 10", hf::format_version::v1);

    ASSERT_STREQ(hfp3.second.get_function(0), "gaus(0)");
    ASSERT_STREQ(hfp3.second.get_function(1), "pol0(3)");

    ASSERT_EQ(hfp3.second.get_fit_range_min(), 1);
    ASSERT_EQ(hfp3.second.get_fit_range_max(), 10);

    ASSERT_EQ(hfp3.second.get_flag_rebin(), 0);
    ASSERT_EQ(hfp3.second.get_flag_disabled(), false);

    ASSERT_EQ(hfp3.second.get_function_params_count(), 4);

    ASSERT_EQ(hfp3.second.param(0).value, 0);
    ASSERT_EQ(hfp3.second.param(0).min, 0);
    ASSERT_EQ(hfp3.second.param(0).max, 0);
    ASSERT_EQ(hfp3.second.param(0).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp3.second.param(0).has_limits, false);

    ASSERT_EQ(hfp3.second.param(1).value, 0);
    ASSERT_EQ(hfp3.second.param(1).min, 0);
    ASSERT_EQ(hfp3.second.param(1).max, 0);
    ASSERT_EQ(hfp3.second.param(1).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp3.second.param(1).has_limits, false);

    ASSERT_EQ(hfp3.second.param(2).value, 0);
    ASSERT_EQ(hfp3.second.param(2).min, 0);
    ASSERT_EQ(hfp3.second.param(2).max, 0);
    ASSERT_EQ(hfp3.second.param(2).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp3.second.param(2).has_limits, false);

    ASSERT_EQ(hfp3.second.param(3).value, 0);
    ASSERT_EQ(hfp3.second.param(3).min, 0);
    ASSERT_EQ(hfp3.second.param(3).max, 0);
    ASSERT_EQ(hfp3.second.param(3).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp3.second.param(3).has_limits, false);

    hfp3.second.print(hfp3.first);

    auto export3 = hf::parser::v1::format_line_entry(hfp3.first, &hfp3.second);
    ASSERT_STREQ(export3, " hist_3\tgaus(0) pol0(3) 0 1 10  0  0  0  0");

    ASSERT_THROW(hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1", hf::format_version::v1), hf::format_error);
}

TEST(TestsParserV1, Parsing2Functions)
{
    auto hfp =
        hf::tools::parse_line_entry("hist_1 gaus(0) expo(3) 0  1 10  1  2 : 1 3  3 F 2 5  4 f", hf::format_version::v1);

    ASSERT_EQ(hfp.second.get_functions_count(), 2);
    ASSERT_STREQ(hfp.second.get_function(0), "gaus(0)");
    ASSERT_STREQ(hfp.second.get_function(1), "expo(3)");
}

TEST(TestsParserV1, ComplexLine)
{
    auto hfp = hf::tools::parse_line_entry("hist_1 [0]+[1]*x [3]+[4]*x 0  1 10  1  2 : 1 3  3 F 2 5");

    ASSERT_EQ(hfp.second.get_functions_count(), 2);
    ASSERT_EQ(hfp.second.get_function_params_count(), 5);
    ASSERT_STREQ(hfp.second.get_function(0), "[0]+[1]*x");
    ASSERT_STREQ(hfp.second.get_function(1), "[3]+[4]*x");

    hfp.second.print(hfp.first);
}

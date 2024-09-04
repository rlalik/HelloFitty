#include <gtest/gtest.h>

#include "hellofitty.hpp"
#include "parser.hpp"

TEST(TestsParserV2, ParsingInvalid)
{
    ASSERT_THROW(hf::tools::parse_line_entry("hist_1 1 10 0 gaus(0) 1  2 : 1 3  3 F 2 5", hf::format_version::v2),
                 hf::format_error);
    ASSERT_THROW(hf::tools::parse_line_entry("hist_1 1 10 gaus(0) | 1  2 : 1 3  3 F 2 5", hf::format_version::v2),
                 hf::format_error);
    ASSERT_THROW(hf::tools::parse_line_entry("hist_1 1 10 0 gaus(0) | | 1  2 : 1 3  3 F 2 5", hf::format_version::v2),
                 hf::format_error);
}

TEST(TestsParserV2, Parsing1Function)
{
    auto hfp = hf::tools::parse_line_entry("hist_1 1 10 0 gaus(0) | 1  2 : 1 3  3 F 2 5", hf::format_version::v2);

    ASSERT_EQ(hfp.second.get_functions_count(), 1);
    ASSERT_STREQ(hfp.second.get_function(0), "gaus(0)");

    ASSERT_EQ(hfp.second.get_function_params_count(), 3);

    ASSERT_EQ(hfp.second.param(0).value, 1);
    ASSERT_EQ(hfp.second.param(1).value, 2);
    ASSERT_EQ(hfp.second.param(2).value, 3);
}

TEST(TestsParserV2, Parsing2Functions)
{
    auto hfp =
        hf::tools::parse_line_entry("hist_1 1 10 0 gaus(0) expo(3) | 1  2 : 1 3  3 F 2 5  4 f", hf::format_version::v2);

    ASSERT_EQ(hfp.second.get_functions_count(), 2);
    ASSERT_STREQ(hfp.second.get_function(0), "gaus(0)");
    ASSERT_STREQ(hfp.second.get_function(1), "expo(3)");

    ASSERT_EQ(hfp.second.get_function_params_count(), 5);

    ASSERT_EQ(hfp.second.param(0).value, 1);
    ASSERT_EQ(hfp.second.param(1).value, 2);
    ASSERT_EQ(hfp.second.param(2).value, 3);

    ASSERT_EQ(hfp.second.param(3).value, 4);
    ASSERT_EQ(hfp.second.param(4).value, 0);
}

TEST(TestsParserV2, Parsing3Functions)
{
    auto hfp = hf::tools::parse_line_entry("hist_1 1 10 0 gaus(0) expo(3) pol1(5) | 1  2 : 1 3  3 F 2 5  4 f 5 6",
                                           hf::format_version::v2);

    ASSERT_EQ(hfp.second.get_functions_count(), 3);
    ASSERT_STREQ(hfp.second.get_function(0), "gaus(0)");
    ASSERT_STREQ(hfp.second.get_function(1), "expo(3)");
    ASSERT_STREQ(hfp.second.get_function(2), "pol1(5)");

    ASSERT_EQ(hfp.second.get_function_params_count(), 7);

    ASSERT_EQ(hfp.second.param(0).value, 1);
    ASSERT_EQ(hfp.second.param(1).value, 2);
    ASSERT_EQ(hfp.second.param(2).value, 3);

    ASSERT_EQ(hfp.second.param(3).value, 4);
    ASSERT_EQ(hfp.second.param(4).value, 5);

    ASSERT_EQ(hfp.second.param(5).value, 6);
    ASSERT_EQ(hfp.second.param(6).value, 0);
}

TEST(TestsParserV2, Formatting3Functions)
{
    auto hfp = hf::tools::parse_line_entry("hist_1 1 10 0 gaus(0) expo(3) pol1(5) | 1  2 : 1 3  3 F 2 5  4 f 5 6",
                                           hf::format_version::v2);

    auto out = hf::tools::format_line_entry(hfp.first, &hfp.second, hf::format_version::v2);

    ASSERT_STREQ(out, " hist_1\t1 10 0 gaus(0) expo(3) pol1(5) |  1  2 : 1 3  3 F 2 5  4 f  5  6  0");
}

TEST(TestsParserV2, ComplexLine)
{
    auto hfp = hf::tools::parse_line_entry("hist_1 1 10 0 [0]+[1]*x [0]+[4]*x [1]+[2]*x | 1  2 : 1 3  3 F 2 5");

    ASSERT_EQ(hfp.second.get_functions_count(), 3);
    ASSERT_EQ(hfp.second.get_function_params_count(), 5);
    ASSERT_STREQ(hfp.second.get_function(0), "[0]+[1]*x");
    ASSERT_STREQ(hfp.second.get_function(1), "[0]+[4]*x");
    ASSERT_STREQ(hfp.second.get_function(2), "[1]+[2]*x");

    hfp.second.print("hist_1");
}

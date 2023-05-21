#include <gtest/gtest.h>

#include <hellofitty.hpp>
#include <parser.hpp>

TEST(TestsParserV1, ParsingLine)
{
    auto hfp0 = hf::tools::parse_line_entry("hist_1 gaus(0) 0  0  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_FALSE(hfp0.get());

    auto hfp1 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  0  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_STREQ(hfp1->get_name(), "hist_1");
    ASSERT_STREQ(hfp1->get_sig_string(), "gaus(0)");
    ASSERT_STREQ(hfp1->get_bkg_string(), "pol0(3)");

    ASSERT_EQ(hfp1->get_fit_range_min(), 1);
    ASSERT_EQ(hfp1->get_fit_range_max(), 10);

    ASSERT_EQ(hfp1->get_flag_rebin(), 0);
    ASSERT_EQ(hfp1->get_flag_disabled(), false);

    ASSERT_EQ(hfp1->get_params_number(), 4);

    ASSERT_EQ(hfp1->get_param(0).value, 1);
    ASSERT_EQ(hfp1->get_param(0).min, 0);
    ASSERT_EQ(hfp1->get_param(0).max, 0);
    ASSERT_EQ(hfp1->get_param(0).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp1->get_param(0).has_limits, false);

    ASSERT_EQ(hfp1->get_param(1).value, 2);
    ASSERT_EQ(hfp1->get_param(1).min, 1);
    ASSERT_EQ(hfp1->get_param(1).max, 3);
    ASSERT_EQ(hfp1->get_param(1).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp1->get_param(1).has_limits, true);

    ASSERT_EQ(hfp1->get_param(2).value, 3);
    ASSERT_EQ(hfp1->get_param(2).min, 2);
    ASSERT_EQ(hfp1->get_param(2).max, 5);
    ASSERT_EQ(hfp1->get_param(2).mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(hfp1->get_param(2).has_limits, true);

    ASSERT_EQ(hfp1->get_param(3).value, 4);
    ASSERT_EQ(hfp1->get_param(3).min, 0);
    ASSERT_EQ(hfp1->get_param(3).max, 0);
    ASSERT_EQ(hfp1->get_param(3).mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(hfp1->get_param(3).has_limits, false);

    hfp1->init();
    hfp1->print();

    auto export1 = hfp1->export_entry();
    ASSERT_STREQ(export1, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp2 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_STREQ(hfp2->get_name(), "hist_1");
    ASSERT_STREQ(hfp2->get_sig_string(), "gaus(0)");
    ASSERT_STREQ(hfp2->get_bkg_string(), "pol0(3)");

    ASSERT_EQ(hfp2->get_fit_range_min(), 1);
    ASSERT_EQ(hfp2->get_fit_range_max(), 10);

    ASSERT_EQ(hfp2->get_flag_rebin(), 0);
    ASSERT_EQ(hfp2->get_flag_disabled(), false);

    ASSERT_EQ(hfp2->get_params_number(), 4);

    ASSERT_EQ(hfp2->get_param(0).value, 1);
    ASSERT_EQ(hfp2->get_param(0).min, 0);
    ASSERT_EQ(hfp2->get_param(0).max, 0);
    ASSERT_EQ(hfp2->get_param(0).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp2->get_param(0).has_limits, false);

    ASSERT_EQ(hfp2->get_param(1).value, 2);
    ASSERT_EQ(hfp2->get_param(1).min, 1);
    ASSERT_EQ(hfp2->get_param(1).max, 3);
    ASSERT_EQ(hfp2->get_param(1).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp2->get_param(1).has_limits, true);

    ASSERT_EQ(hfp2->get_param(2).value, 3);
    ASSERT_EQ(hfp2->get_param(2).min, 2);
    ASSERT_EQ(hfp2->get_param(2).max, 5);
    ASSERT_EQ(hfp2->get_param(2).mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(hfp2->get_param(2).has_limits, true);

    ASSERT_EQ(hfp2->get_param(3).value, 4);
    ASSERT_EQ(hfp2->get_param(3).min, 0);
    ASSERT_EQ(hfp2->get_param(3).max, 0);
    ASSERT_EQ(hfp2->get_param(3).mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(hfp2->get_param(3).has_limits, false);

    hfp2->init();
    hfp2->print();

    auto export2 = hf::parser::format_line_entry_v1(hfp2.get());
    ASSERT_STREQ(export2, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp3 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1 10", 0);

    ASSERT_STREQ(hfp3->get_name(), "hist_1");
    ASSERT_STREQ(hfp3->get_sig_string(), "gaus(0)");
    ASSERT_STREQ(hfp3->get_bkg_string(), "pol0(3)");

    ASSERT_EQ(hfp3->get_fit_range_min(), 1);
    ASSERT_EQ(hfp3->get_fit_range_max(), 10);

    ASSERT_EQ(hfp3->get_flag_rebin(), 0);
    ASSERT_EQ(hfp3->get_flag_disabled(), false);

    ASSERT_EQ(hfp3->get_params_number(), 4);

    ASSERT_EQ(hfp3->get_param(0).value, 0);
    ASSERT_EQ(hfp3->get_param(0).min, 0);
    ASSERT_EQ(hfp3->get_param(0).max, 0);
    ASSERT_EQ(hfp3->get_param(0).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp3->get_param(0).has_limits, false);

    ASSERT_EQ(hfp3->get_param(1).value, 0);
    ASSERT_EQ(hfp3->get_param(1).min, 0);
    ASSERT_EQ(hfp3->get_param(1).max, 0);
    ASSERT_EQ(hfp3->get_param(1).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp3->get_param(1).has_limits, false);

    ASSERT_EQ(hfp3->get_param(2).value, 0);
    ASSERT_EQ(hfp3->get_param(2).min, 0);
    ASSERT_EQ(hfp3->get_param(2).max, 0);
    ASSERT_EQ(hfp3->get_param(2).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp3->get_param(2).has_limits, false);

    ASSERT_EQ(hfp3->get_param(3).value, 0);
    ASSERT_EQ(hfp3->get_param(3).min, 0);
    ASSERT_EQ(hfp3->get_param(3).max, 0);
    ASSERT_EQ(hfp3->get_param(3).mode, hf::param::fit_mode::free);
    ASSERT_EQ(hfp3->get_param(3).has_limits, false);

    hfp3->init();
    hfp3->print();

    auto export3 = hf::parser::format_line_entry_v1(hfp3.get());
    ASSERT_STREQ(export3, " hist_1\tgaus(0) pol0(3) 0 1 10 0 0 0 0");

    auto hfp4 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1", 0);

    ASSERT_FALSE(hfp4.get());
}

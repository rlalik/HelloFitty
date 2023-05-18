#include <gtest/gtest.h>

#include "fitemall.hpp"

using fea::histogram_fit;

TEST(tests_histogram_fit, basic)
{
    histogram_fit hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    ASSERT_STREQ(hfp1.get_name(), "h1");
    ASSERT_STREQ(hfp1.get_sig_string(), "gaus(0)");
    ASSERT_STREQ(hfp1.get_bkg_string(), "expo(3)");

    ASSERT_EQ(hfp1.get_fit_range_l(), 1);
    ASSERT_EQ(hfp1.get_fit_range_u(), 10);

    ASSERT_EQ(hfp1.get_flag_rebin(), 0);
    ASSERT_EQ(hfp1.get_flag_disabled(), false);
}

TEST(tests_histogram_fit, parsing_line)
{
    auto hfp0 =
        fea::tools::parse_line_entry("hist_1 gaus(0) 0  0  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_FALSE(hfp0.get());

    auto hfp1 = fea::tools::parse_line_entry(
        "hist_1 gaus(0) pol0(3)  0  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_STREQ(hfp1->get_name(), "hist_1");
    ASSERT_STREQ(hfp1->get_sig_string(), "gaus(0)");
    ASSERT_STREQ(hfp1->get_bkg_string(), "pol0(3)");

    ASSERT_EQ(hfp1->get_fit_range_l(), 1);
    ASSERT_EQ(hfp1->get_fit_range_u(), 10);

    ASSERT_EQ(hfp1->get_flag_rebin(), 0);
    ASSERT_EQ(hfp1->get_flag_disabled(), false);

    ASSERT_EQ(hfp1->get_params_number(), 4);

    ASSERT_EQ(hfp1->get_param(0).value, 1);
    ASSERT_EQ(hfp1->get_param(0).lower, 0);
    ASSERT_EQ(hfp1->get_param(0).upper, 0);
    ASSERT_EQ(hfp1->get_param(0).mode, fea::param::fit_mode::free);
    ASSERT_EQ(hfp1->get_param(0).has_limits, false);

    ASSERT_EQ(hfp1->get_param(1).value, 2);
    ASSERT_EQ(hfp1->get_param(1).lower, 1);
    ASSERT_EQ(hfp1->get_param(1).upper, 3);
    ASSERT_EQ(hfp1->get_param(1).mode, fea::param::fit_mode::free);
    ASSERT_EQ(hfp1->get_param(1).has_limits, true);

    ASSERT_EQ(hfp1->get_param(2).value, 3);
    ASSERT_EQ(hfp1->get_param(2).lower, 2);
    ASSERT_EQ(hfp1->get_param(2).upper, 5);
    ASSERT_EQ(hfp1->get_param(2).mode, fea::param::fit_mode::fixed);
    ASSERT_EQ(hfp1->get_param(2).has_limits, true);

    ASSERT_EQ(hfp1->get_param(3).value, 4);
    ASSERT_EQ(hfp1->get_param(3).lower, 0);
    ASSERT_EQ(hfp1->get_param(3).upper, 0);
    ASSERT_EQ(hfp1->get_param(3).mode, fea::param::fit_mode::fixed);
    ASSERT_EQ(hfp1->get_param(3).has_limits, false);

    hfp1->init();
    hfp1->print();

    auto export1 = hfp1->export_entry();
    ASSERT_STREQ(export1, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp2 = fea::tools::parse_line_entry(
        "hist_1 gaus(0) pol0(3)  1  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_STREQ(hfp2->get_name(), "hist_1");
    ASSERT_STREQ(hfp2->get_sig_string(), "gaus(0)");
    ASSERT_STREQ(hfp2->get_bkg_string(), "pol0(3)");

    ASSERT_EQ(hfp2->get_fit_range_l(), 1);
    ASSERT_EQ(hfp2->get_fit_range_u(), 10);

    ASSERT_EQ(hfp2->get_flag_rebin(), 0);
    ASSERT_EQ(hfp2->get_flag_disabled(), false);

    ASSERT_EQ(hfp2->get_params_number(), 4);

    ASSERT_EQ(hfp2->get_param(0).value, 1);
    ASSERT_EQ(hfp2->get_param(0).lower, 0);
    ASSERT_EQ(hfp2->get_param(0).upper, 0);
    ASSERT_EQ(hfp2->get_param(0).mode, fea::param::fit_mode::free);
    ASSERT_EQ(hfp2->get_param(0).has_limits, false);

    ASSERT_EQ(hfp2->get_param(1).value, 2);
    ASSERT_EQ(hfp2->get_param(1).lower, 1);
    ASSERT_EQ(hfp2->get_param(1).upper, 3);
    ASSERT_EQ(hfp2->get_param(1).mode, fea::param::fit_mode::free);
    ASSERT_EQ(hfp2->get_param(1).has_limits, true);

    ASSERT_EQ(hfp2->get_param(2).value, 3);
    ASSERT_EQ(hfp2->get_param(2).lower, 2);
    ASSERT_EQ(hfp2->get_param(2).upper, 5);
    ASSERT_EQ(hfp2->get_param(2).mode, fea::param::fit_mode::fixed);
    ASSERT_EQ(hfp2->get_param(2).has_limits, true);

    ASSERT_EQ(hfp2->get_param(3).value, 4);
    ASSERT_EQ(hfp2->get_param(3).lower, 0);
    ASSERT_EQ(hfp2->get_param(3).upper, 0);
    ASSERT_EQ(hfp2->get_param(3).mode, fea::param::fit_mode::fixed);
    ASSERT_EQ(hfp2->get_param(3).has_limits, false);

    hfp2->init();
    hfp2->print();

    auto export2 = hfp2->export_entry();
    ASSERT_STREQ(export2, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp3 = fea::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1 10", 0);

    ASSERT_STREQ(hfp3->get_name(), "hist_1");
    ASSERT_STREQ(hfp3->get_sig_string(), "gaus(0)");
    ASSERT_STREQ(hfp3->get_bkg_string(), "pol0(3)");

    ASSERT_EQ(hfp3->get_fit_range_l(), 1);
    ASSERT_EQ(hfp3->get_fit_range_u(), 10);

    ASSERT_EQ(hfp3->get_flag_rebin(), 0);
    ASSERT_EQ(hfp3->get_flag_disabled(), false);

    ASSERT_EQ(hfp3->get_params_number(), 4);

    ASSERT_EQ(hfp3->get_param(0).value, 0);
    ASSERT_EQ(hfp3->get_param(0).lower, 0);
    ASSERT_EQ(hfp3->get_param(0).upper, 0);
    ASSERT_EQ(hfp3->get_param(0).mode, fea::param::fit_mode::free);
    ASSERT_EQ(hfp3->get_param(0).has_limits, false);

    ASSERT_EQ(hfp3->get_param(1).value, 0);
    ASSERT_EQ(hfp3->get_param(1).lower, 0);
    ASSERT_EQ(hfp3->get_param(1).upper, 0);
    ASSERT_EQ(hfp3->get_param(1).mode, fea::param::fit_mode::free);
    ASSERT_EQ(hfp3->get_param(1).has_limits, false);

    ASSERT_EQ(hfp3->get_param(2).value, 0);
    ASSERT_EQ(hfp3->get_param(2).lower, 0);
    ASSERT_EQ(hfp3->get_param(2).upper, 0);
    ASSERT_EQ(hfp3->get_param(2).mode, fea::param::fit_mode::free);
    ASSERT_EQ(hfp3->get_param(2).has_limits, false);

    ASSERT_EQ(hfp3->get_param(3).value, 0);
    ASSERT_EQ(hfp3->get_param(3).lower, 0);
    ASSERT_EQ(hfp3->get_param(3).upper, 0);
    ASSERT_EQ(hfp3->get_param(3).mode, fea::param::fit_mode::free);
    ASSERT_EQ(hfp3->get_param(3).has_limits, false);

    hfp3->init();
    hfp3->print();

    auto export3 = hfp3->export_entry();
    ASSERT_STREQ(export3, " hist_1\tgaus(0) pol0(3) 0 1 10 0 0 0 0");

    auto hfp4 = fea::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1", 0);

    ASSERT_FALSE(hfp4.get());
}

TEST(tests_histogram_fit, cloning)
{
    histogram_fit hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    auto hfp2 = hfp1.clone("h2");

    ASSERT_STRNE(hfp1.get_name(), hfp2->get_name());
    ASSERT_STREQ(hfp2->get_name(), "h2");
    ASSERT_STREQ(hfp1.get_sig_string(), hfp2->get_sig_string());
    ASSERT_STREQ(hfp1.get_bkg_string(), hfp2->get_bkg_string());

    ASSERT_EQ(hfp1.get_fit_range_l(), hfp2->get_fit_range_l());
    ASSERT_EQ(hfp1.get_fit_range_u(), hfp2->get_fit_range_u());

    ASSERT_EQ(hfp1.get_flag_rebin(), hfp2->get_flag_rebin());
    ASSERT_EQ(hfp1.get_flag_disabled(), hfp2->get_flag_disabled());

    hfp1.push();
    hfp1.clear();
}

TEST(tests_histogram_fit, backups)
{
    histogram_fit hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    fea::param p1;
    auto p2 = fea::param();
    auto p3 = fea::param(3, fea::param::fit_mode::fixed);
    auto p4 = fea::param(4, 1, 10, fea::param::fit_mode::free);

    ASSERT_EQ(hfp1.get_params_number(), 5);

    hfp1.set_param(0, p1);
    hfp1.set_param(1, p2);
    hfp1.set_param(2, p3);
    hfp1.set_param(3, p4);

    // try {
    //     hfp1.set_param(5, p4);
    // } catch (...) {
    //     ASSERT_NE(hfp1.get_paramsNumber(), 5);
    // }

    hfp1.push();

    ASSERT_EQ(hfp1.get_param(0).value, 0);
    ASSERT_EQ(hfp1.get_param(1).value, 0);
    ASSERT_EQ(hfp1.get_param(2).value, 3);
    ASSERT_EQ(hfp1.get_param(3).value, 4);

    hfp1.update_param(0, 10);
    hfp1.update_param(1, 20);
    hfp1.update_param(2, 30);
    hfp1.update_param(3, 40);

    hfp1.apply();

    ASSERT_EQ(hfp1.get_param(0).value, 0);
    ASSERT_EQ(hfp1.get_param(1).value, 0);
    ASSERT_EQ(hfp1.get_param(2).value, 3);
    ASSERT_EQ(hfp1.get_param(3).value, 4);

    hfp1.update_param(0, 10);
    hfp1.update_param(1, 20);
    hfp1.update_param(2, 30);
    hfp1.update_param(3, 40);

    hfp1.pop();

    ASSERT_EQ(hfp1.get_param(0).value, 0);
    ASSERT_EQ(hfp1.get_param(1).value, 0);
    ASSERT_EQ(hfp1.get_param(2).value, 3);
    ASSERT_EQ(hfp1.get_param(3).value, 4);

    hfp1.update_param(0, 10);
    hfp1.update_param(1, 20);
    hfp1.update_param(2, 30);
    hfp1.update_param(3, 40);

    hfp1.pop();

    ASSERT_EQ(hfp1.get_param(0).value, 10);
    ASSERT_EQ(hfp1.get_param(1).value, 20);
    ASSERT_EQ(hfp1.get_param(2).value, 30);
    ASSERT_EQ(hfp1.get_param(3).value, 40);

    hfp1.drop();
}

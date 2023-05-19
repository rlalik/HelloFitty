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

    auto export1 = hfp1->export_entry();
    ASSERT_STREQ(export1, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp2 = fea::tools::parse_line_entry(
        "hist_1 gaus(0) pol0(3)  1  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    auto export2 = hfp2->export_entry();
    ASSERT_STREQ(export2, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp3 = fea::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1 10", 0);

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

    hfp1.save();
    hfp1.clear();
}

TEST(tests_histogram_fit, backups)
{
    histogram_fit hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    std::tuple<int, int> test_values_1[] = {
        // clang-format: off
        {0, 0},
        {1, 0},
        {2, 3},
        {3, 4}
        // clang-format: on
    };

    std::tuple<int, int> test_values_2[] = {
        // clang-format: off
        {0, 10},
        {1, 20},
        {2, 30},
        {3, 40}
        // clang-format: on
    };

    std::tuple<int, int> test_values_3[] = {
        // clang-format: off
        {0, 100},
        {1, 200},
        {2, 300},
        {3, 400}
        // clang-format: on
    };

    fea::param p0;
    auto p1 = fea::param();
    auto p2 = fea::param(3, fea::param::fit_mode::fixed);
    auto p3 = fea::param(4, 1, 10, fea::param::fit_mode::free);

    ASSERT_EQ(hfp1.get_params_number(), 5);

    hfp1.set_param(0, p0);
    hfp1.set_param(1, p1);
    hfp1.set_param(2, p2);
    hfp1.set_param(3, p3);

    // try {
    //     hfp1.set_param(5, p4);
    // } catch (...) {
    //     ASSERT_NE(hfp1.get_paramsNumber(), 5);
    // }

    // save test_values_1
    hfp1.save();

    // should still contain test_values_1
    for (auto& test_data : test_values_1)
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));

    // set to test_values_2
    for (auto& test_data : test_values_2)
        hfp1.update_param(std::get<0>(test_data), std::get<1>(test_data));

    // should read test_values_2
    for (auto& test_data : test_values_2)
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));

    // should load test_values_1
    hfp1.load();

    // should read test_values_1
    for (auto& test_data : test_values_1)
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));

    // should set test_values_2
    for (auto& test_data : test_values_3)
        hfp1.update_param(std::get<0>(test_data), std::get<1>(test_data));

    // should load test_values_1
    hfp1.load();

    // should read test_values_1
    for (auto& test_data : test_values_1)
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));

    for (auto& test_data : test_values_3)
        hfp1.update_param(std::get<0>(test_data), std::get<1>(test_data));

    // clear backup storage
    hfp1.drop();

    // should not allow to load from empty
    ASSERT_THROW(hfp1.load(), std::out_of_range);

    // still test_values_2
    for (auto& test_data : test_values_3)
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));

    hfp1.drop();
}

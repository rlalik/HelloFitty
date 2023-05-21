#include <gtest/gtest.h>

#include <hellofitty.hpp>

#include <TString.h> // for TString

#include <memory>    // for unique_ptr, allocator
#include <stdexcept> // for out_of_range
#include <string>    // for string
#include <tuple>     // for get, tuple

TEST(TestsFitEntry, Basic)
{
    hf::fit_entry hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    ASSERT_STREQ(hfp1.get_name(), "h1");
    ASSERT_STREQ(hfp1.get_sig_string(), "gaus(0)");
    ASSERT_STREQ(hfp1.get_bkg_string(), "expo(3)");

    ASSERT_EQ(hfp1.get_fit_range_min(), 1);
    ASSERT_EQ(hfp1.get_fit_range_max(), 10);

    ASSERT_EQ(hfp1.get_flag_rebin(), 0);
    ASSERT_EQ(hfp1.get_flag_disabled(), false);
}

TEST(TestsFitEntry, ParsingLine)
{
    auto hfp0 = hf::tools::parse_line_entry("hist_1 gaus(0) 0  0  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_FALSE(hfp0.get());

    auto hfp1 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  0  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    auto export1 = hfp1->export_entry();
    ASSERT_STREQ(export1, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp2 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    auto export2 = hfp2->export_entry();
    ASSERT_STREQ(export2, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp3 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1 10", 0);

    auto export3 = hfp3->export_entry();
    ASSERT_STREQ(export3, " hist_1\tgaus(0) pol0(3) 0 1 10 0 0 0 0");

    auto hfp4 = hf::tools::parse_line_entry("hist_1 gaus(0) pol0(3)  1  1", 0);

    ASSERT_FALSE(hfp4.get());
}

TEST(TestsFitEntry, Cloning)
{
    hf::fit_entry hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    auto hfp2 = hfp1.clone("h2");

    ASSERT_STRNE(hfp1.get_name(), hfp2->get_name());
    ASSERT_STREQ(hfp2->get_name(), "h2");
    ASSERT_STREQ(hfp1.get_sig_string(), hfp2->get_sig_string());
    ASSERT_STREQ(hfp1.get_bkg_string(), hfp2->get_bkg_string());

    ASSERT_EQ(hfp1.get_fit_range_min(), hfp2->get_fit_range_min());
    ASSERT_EQ(hfp1.get_fit_range_max(), hfp2->get_fit_range_max());

    ASSERT_EQ(hfp1.get_flag_rebin(), hfp2->get_flag_rebin());
    ASSERT_EQ(hfp1.get_flag_disabled(), hfp2->get_flag_disabled());

    hfp1.backup();
    hfp1.clear();
}

TEST(TestsFitEntry, Backups)
{
    hf::fit_entry hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    std::tuple<int, int> test_values_1[] = {
        // clang-format: off
        {0, 0}, // NOLINT
        {1, 0}, // NOLINT
        {2, 3}, // NOLINT
        {3, 4}  // NOLINT
        // clang-format: on
    };

    std::tuple<int, int> test_values_2[] = {
        // clang-format: off
        {0, 10}, // NOLINT
        {1, 20}, // NOLINT
        {2, 30}, // NOLINT
        {3, 40}  // NOLINT
        // clang-format: on
    };

    std::tuple<int, int> test_values_3[] = {
        // clang-format: off
        {0, 100}, // NOLINT
        {1, 200}, // NOLINT
        {2, 300}, // NOLINT
        {3, 400}  // NOLINT
        // clang-format: on
    };

    const hf::param par0;
    const auto par1 = hf::param();
    const auto par2 = hf::param(3, hf::param::fit_mode::fixed);
    const auto par3 = hf::param(4, 1, 10, hf::param::fit_mode::free); // NOLINT

    ASSERT_EQ(hfp1.get_params_number(), 5);

    hfp1.set_param(0, par0);
    hfp1.set_param(1, par1);
    hfp1.set_param(2, par2);
    hfp1.set_param(3, par3);

    // try {
    //     hfp1.set_param(5, p4);
    // } catch (...) {
    //     ASSERT_NE(hfp1.get_paramsNumber(), 5);
    // }

    // backup test_values_1
    hfp1.backup();

    // should still contain test_values_1
    for (auto& test_data : test_values_1)
    {
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));
    }

    // set to test_values_2
    for (auto& test_data : test_values_2)
    {
        hfp1.update_param(std::get<0>(test_data), std::get<1>(test_data));
    }

    // should read test_values_2
    for (auto& test_data : test_values_2)
    {
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));
    }

    // should restore test_values_1
    hfp1.restore();

    // should read test_values_1
    for (auto& test_data : test_values_1)
    {
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));
    }

    // should set test_values_2
    for (auto& test_data : test_values_3)
    {
        hfp1.update_param(std::get<0>(test_data), std::get<1>(test_data));
    }

    // should restore test_values_1
    hfp1.restore();

    // should read test_values_1
    for (auto& test_data : test_values_1)
    {
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));
    }

    for (auto& test_data : test_values_3)
    {
        hfp1.update_param(std::get<0>(test_data), std::get<1>(test_data));
    }

    // clear backup storage
    hfp1.drop();

    // should not allow to restore from empty
    ASSERT_THROW(hfp1.restore(), std::out_of_range);

    // still test_values_2
    for (auto& test_data : test_values_3)
    {
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));
    }

    hfp1.drop();
}

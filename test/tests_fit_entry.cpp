#include <gtest/gtest.h>

#include "hellofitty.hpp"

#include <TString.h> // for TString

#include <memory>    // for unique_ptr, allocator
#include <stdexcept> // for out_of_range
#include <string>    // for string
#include <tuple>     // for get, tuple

TEST(TestsFitEntry, Functions)
{
    hf::fit_entry hfp1("h1", 1, 10);
    ASSERT_STREQ(hfp1.get_name(), "h1");
    ASSERT_EQ(hfp1.get_fit_range_min(), 1);
    ASSERT_EQ(hfp1.get_fit_range_max(), 10);

    ASSERT_EQ(hfp1.get_flag_rebin(), 0);
    ASSERT_EQ(hfp1.get_flag_disabled(), false);

    ASSERT_EQ(hfp1.get_functions_count(), 0);

    ASSERT_EQ(hfp1.add_function("gaus(0)"), 0);
    ASSERT_EQ(hfp1.get_functions_count(), 1);

    ASSERT_EQ(hfp1.add_function("expo(3)"), 1);
    ASSERT_EQ(hfp1.get_functions_count(), 2);

    ASSERT_EQ(hfp1.add_function("pol1(5)"), 2);
    ASSERT_EQ(hfp1.get_functions_count(), 3);

    ASSERT_STREQ(hfp1.get_function(0), "gaus(0)");
    ASSERT_STREQ(hfp1.get_function(1), "expo(3)");
}

TEST(TestsFitEntry, Params)
{
    hf::fit_entry hfp1("h1", 1, 10);
    ASSERT_EQ(hfp1.add_function("gaus(0)"), 0);

    ASSERT_NO_THROW(hfp1.get_param(0));
    ASSERT_THROW(hfp1.get_param(10), hf::index_error);

    hfp1.set_param(0, 13, hf::param::fit_mode::free);

    ASSERT_NO_THROW(hfp1.get_param("Constant"));
    ASSERT_THROW(hfp1.get_param("Foo"), hf::index_error);

    ASSERT_EQ(hfp1.get_param("Constant").value, 13);

    const auto& hfp2 = hfp1;

    ASSERT_EQ(hfp2.get_param("Constant").value, 13);
}

TEST(TestsFitEntry, Cloning)
{
    hf::fit_entry hfp1("h1", 1, 10);
    ASSERT_EQ(hfp1.add_function("gaus(0)"), 0);
    ASSERT_EQ(hfp1.add_function("expo(3)"), 1);

    auto hfp2 = hfp1.clone("h2");

    ASSERT_STRNE(hfp1.get_name(), hfp2->get_name());
    ASSERT_STREQ(hfp2->get_name(), "h2");

    for (int i = 0; i < hfp1.get_functions_count(); ++i)
        ASSERT_STREQ(hfp1.get_function(i), hfp2->get_function(i));

    ASSERT_EQ(hfp1.get_fit_range_min(), hfp2->get_fit_range_min());
    ASSERT_EQ(hfp1.get_fit_range_max(), hfp2->get_fit_range_max());

    ASSERT_EQ(hfp1.get_flag_rebin(), hfp2->get_flag_rebin());
    ASSERT_EQ(hfp1.get_flag_disabled(), hfp2->get_flag_disabled());

    hfp1.backup();
    hfp1.clear();
}

TEST(TestsFitEntry, Backups)
{
    hf::fit_entry hfp1("h1", 1, 10);
    ASSERT_EQ(hfp1.add_function("gaus(0)"), 0);
    ASSERT_EQ(hfp1.add_function("expo(3)"), 1);

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

    ASSERT_EQ(hfp1.get_function_params_count(), 5);

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
    ASSERT_THROW(hfp1.restore(), hf::length_error);

    // still test_values_2
    for (auto& test_data : test_values_3)
    {
        ASSERT_EQ(hfp1.get_param(std::get<0>(test_data)).value, std::get<1>(test_data));
    }

    hfp1.drop();
}

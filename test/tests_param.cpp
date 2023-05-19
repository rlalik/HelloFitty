#include <gtest/gtest.h>

#include "hellofitty.hpp"

#include <memory>

TEST(tests_Param, basic)
{
    hf::param p1;

    ASSERT_EQ(p1.value, 0);
    ASSERT_EQ(p1.lower, 0);
    ASSERT_EQ(p1.upper, 0);
    ASSERT_EQ(p1.mode, hf::param::fit_mode::free);
    ASSERT_EQ(p1.has_limits, false);

    auto p2 = hf::param();

    ASSERT_EQ(p2.value, 0);
    ASSERT_EQ(p2.lower, 0);
    ASSERT_EQ(p2.upper, 0);
    ASSERT_EQ(p2.mode, hf::param::fit_mode::free);
    ASSERT_EQ(p2.has_limits, false);

    auto p3 = hf::param(3, hf::param::fit_mode::fixed);

    ASSERT_EQ(p3.value, 3);
    ASSERT_EQ(p3.lower, 0);
    ASSERT_EQ(p3.upper, 0);
    ASSERT_EQ(p3.mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(p3.has_limits, false);

    auto p4 = hf::param(4, 1, 10, hf::param::fit_mode::free);

    ASSERT_EQ(p4.value, 4);
    ASSERT_EQ(p4.lower, 1);
    ASSERT_EQ(p4.upper, 10);
    ASSERT_EQ(p4.mode, hf::param::fit_mode::free);
    ASSERT_EQ(p4.has_limits, true);
}

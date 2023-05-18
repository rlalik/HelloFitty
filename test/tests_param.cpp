#include <gtest/gtest.h>

#include "fitemall.hpp"

#include <memory>

TEST(tests_Param, basic)
{
    fea::param p1;

    ASSERT_EQ(p1.value, 0);
    ASSERT_EQ(p1.lower, 0);
    ASSERT_EQ(p1.upper, 0);
    ASSERT_EQ(p1.mode, fea::param::fit_mode::free);
    ASSERT_EQ(p1.has_limits, false);

    auto p2 = fea::param();

    ASSERT_EQ(p2.value, 0);
    ASSERT_EQ(p2.lower, 0);
    ASSERT_EQ(p2.upper, 0);
    ASSERT_EQ(p2.mode, fea::param::fit_mode::free);
    ASSERT_EQ(p2.has_limits, false);

    auto p3 = fea::param(3, fea::param::fit_mode::fixed);

    ASSERT_EQ(p3.value, 3);
    ASSERT_EQ(p3.lower, 0);
    ASSERT_EQ(p3.upper, 0);
    ASSERT_EQ(p3.mode, fea::param::fit_mode::fixed);
    ASSERT_EQ(p3.has_limits, false);

    auto p4 = fea::param(4, 1, 10, fea::param::fit_mode::free);

    ASSERT_EQ(p4.value, 4);
    ASSERT_EQ(p4.lower, 1);
    ASSERT_EQ(p4.upper, 10);
    ASSERT_EQ(p4.mode, fea::param::fit_mode::free);
    ASSERT_EQ(p4.has_limits, true);
}

#include <gtest/gtest.h>

#include "FitterFactory.h"

TEST(tests_Param, basic)
{
    FF::Param p1;

    ASSERT_EQ(p1.value, 0);
    ASSERT_EQ(p1.lower, 0);
    ASSERT_EQ(p1.upper, 0);
    ASSERT_EQ(p1.mode, FF::Param::FitMode::Free);
    ASSERT_EQ(p1.has_limits, false);

    auto p2 = FF::Param();

    ASSERT_EQ(p2.value, 0);
    ASSERT_EQ(p2.lower, 0);
    ASSERT_EQ(p2.upper, 0);
    ASSERT_EQ(p2.mode, FF::Param::FitMode::Free);
    ASSERT_EQ(p2.has_limits, false);

    auto p3 = FF::Param(3, FF::Param::FitMode::Fixed);

    ASSERT_EQ(p3.value, 3);
    ASSERT_EQ(p3.lower, 0);
    ASSERT_EQ(p3.upper, 0);
    ASSERT_EQ(p3.mode, FF::Param::FitMode::Fixed);
    ASSERT_EQ(p3.has_limits, false);

    auto p4 = FF::Param(4, 1, 10, FF::Param::FitMode::Free);

    ASSERT_EQ(p4.value, 4);
    ASSERT_EQ(p4.lower, 1);
    ASSERT_EQ(p4.upper, 10);
    ASSERT_EQ(p4.mode, FF::Param::FitMode::Free);
    ASSERT_EQ(p4.has_limits, true);
}

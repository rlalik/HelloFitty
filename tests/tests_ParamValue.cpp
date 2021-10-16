#include <gtest/gtest.h>

#include "FitterFactory.h"

TEST(tests_ParamValue, basic)
{
    ParamValue p1;

    ASSERT_EQ(p1.val, 0);
    ASSERT_EQ(p1.l, 0);
    ASSERT_EQ(p1.u, 0);
    ASSERT_EQ(p1.mode, ParamValue::FitMode::Free);
    ASSERT_EQ(p1.has_limits, false);

    auto p2 = ParamValue();

    ASSERT_EQ(p2.val, 0);
    ASSERT_EQ(p2.l, 0);
    ASSERT_EQ(p2.u, 0);
    ASSERT_EQ(p2.mode, ParamValue::FitMode::Free);
    ASSERT_EQ(p2.has_limits, false);

    auto p3 = ParamValue(3, ParamValue::FitMode::Fixed);

    ASSERT_EQ(p3.val, 3);
    ASSERT_EQ(p3.l, 0);
    ASSERT_EQ(p3.u, 0);
    ASSERT_EQ(p3.mode, ParamValue::FitMode::Fixed);
    ASSERT_EQ(p3.has_limits, false);

    auto p4 = ParamValue(4, 1, 10, ParamValue::FitMode::Free);

    ASSERT_EQ(p4.val, 4);
    ASSERT_EQ(p4.l, 1);
    ASSERT_EQ(p4.u, 10);
    ASSERT_EQ(p4.mode, ParamValue::FitMode::Free);
    ASSERT_EQ(p4.has_limits, true);
}

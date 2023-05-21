#include <gtest/gtest.h>

#include <hellofitty.hpp>

#include <memory>

TEST(TestsParam, Basic)
{
    const hf::param par1;

    ASSERT_EQ(par1.value, 0);
    ASSERT_EQ(par1.min, 0);
    ASSERT_EQ(par1.max, 0);
    ASSERT_EQ(par1.mode, hf::param::fit_mode::free);
    ASSERT_EQ(par1.has_limits, false);

    const auto par2 = hf::param();

    ASSERT_EQ(par2.value, 0);
    ASSERT_EQ(par2.min, 0);
    ASSERT_EQ(par2.max, 0);
    ASSERT_EQ(par2.mode, hf::param::fit_mode::free);
    ASSERT_EQ(par2.has_limits, false);

    const auto par3 = hf::param(3, hf::param::fit_mode::fixed);

    ASSERT_EQ(par3.value, 3);
    ASSERT_EQ(par3.min, 0);
    ASSERT_EQ(par3.max, 0);
    ASSERT_EQ(par3.mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(par3.has_limits, false);

    const auto par4 = hf::param(4, 1, 10, hf::param::fit_mode::free);

    ASSERT_EQ(par4.value, 4);
    ASSERT_EQ(par4.min, 1);
    ASSERT_EQ(par4.max, 10);
    ASSERT_EQ(par4.mode, hf::param::fit_mode::free);
    ASSERT_EQ(par4.has_limits, true);
}

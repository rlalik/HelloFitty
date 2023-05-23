#include <gtest/gtest.h>

#include <fmt/ranges.h>

#include <hellofitty.hpp>

#include <memory>

const hf::param par1;
const auto par2 = hf::param();
const auto par3 = hf::param(3, hf::param::fit_mode::fixed);
const auto par4 = hf::param(4, hf::param::fit_mode::free);
const auto par5 = hf::param(5, 1, 10, hf::param::fit_mode::fixed);
const auto par6 = hf::param(6.1, 1e-6, 10.1, hf::param::fit_mode::free);

TEST(TestsParam, Basic)
{
    ASSERT_EQ(par1.value, 0);
    ASSERT_EQ(par1.min, 0);
    ASSERT_EQ(par1.max, 0);
    ASSERT_EQ(par1.mode, hf::param::fit_mode::free);
    ASSERT_EQ(par1.has_limits, false);

    ASSERT_EQ(par2.value, 0);
    ASSERT_EQ(par2.min, 0);
    ASSERT_EQ(par2.max, 0);
    ASSERT_EQ(par2.mode, hf::param::fit_mode::free);
    ASSERT_EQ(par2.has_limits, false);

    ASSERT_EQ(par3.value, 3);
    ASSERT_EQ(par3.min, 0);
    ASSERT_EQ(par3.max, 0);
    ASSERT_EQ(par3.mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(par3.has_limits, false);

    ASSERT_EQ(par4.value, 4);
    ASSERT_EQ(par4.min, 0);
    ASSERT_EQ(par4.max, 0);
    ASSERT_EQ(par4.mode, hf::param::fit_mode::free);
    ASSERT_EQ(par4.has_limits, false);

    ASSERT_EQ(par5.value, 5);
    ASSERT_EQ(par5.min, 1);
    ASSERT_EQ(par5.max, 10);
    ASSERT_EQ(par5.mode, hf::param::fit_mode::fixed);
    ASSERT_EQ(par5.has_limits, true);

    ASSERT_EQ(par6.value, 6.1);
    ASSERT_EQ(par6.min, 1e-6);
    ASSERT_EQ(par6.max, 10.1);
    ASSERT_EQ(par6.mode, hf::param::fit_mode::free);
    ASSERT_EQ(par6.has_limits, true);
}

TEST(TestsParam, FormatterE) { ASSERT_THROW(fmt::format("{:e}", par1), fmt::format_error); }

/*
TEST(TestsParam, FormatterE)
{
    std::string fmt1 = fmt::format("{:e}", par1);
    ASSERT_EQ(fmt1, "0");

    std::string fmt2 = fmt::format("{:e}", par2);
    ASSERT_EQ(fmt2, "0");

    std::string fmt3 = fmt::format("{:e}", par3);
    ASSERT_EQ(fmt3, "3 f");

    std::string fmt4 = fmt::format("{:e}", par4);
    ASSERT_EQ(fmt4, "4");

    std::string fmt5 = fmt::format("{:e}", par5);
    ASSERT_EQ(fmt5, "5 F 1 10");

    std::string fmt6 = fmt::format("{:e}", par6);
    ASSERT_EQ(fmt6, "6.1 : 1e-06 10.1");
}

TEST(TestsParam, FormatterF)
{
    std::string fmt1 = fmt::format("{:f}", par1);
    ASSERT_EQ(fmt1, "0");

    std::string fmt2 = fmt::format("{:f}", par2);
    ASSERT_EQ(fmt2, "0");

    std::string fmt3 = fmt::format("{:f}", par3);
    ASSERT_EQ(fmt3, "3 f");

    std::string fmt4 = fmt::format("{:f}", par4);
    ASSERT_EQ(fmt4, "4");

    std::string fmt5 = fmt::format("{:f}", par5);
    ASSERT_EQ(fmt5, "5 F 1 10");

    std::string fmt6 = fmt::format("{:f}", par6);
    ASSERT_EQ(fmt6, "6.1 : 0.000001 10.1");
}

TEST(TestsParam, FormatterG)
{
    std::string fmt1 = fmt::format("{:g}", par1);
    ASSERT_EQ(fmt1, "0");

    std::string fmt2 = fmt::format("{:g}", par2);
    ASSERT_EQ(fmt2, "0");

    std::string fmt3 = fmt::format("{:g}", par3);
    ASSERT_EQ(fmt3, "3 f");

    std::string fmt4 = fmt::format("{:g}", par4);
    ASSERT_EQ(fmt4, "4");

    std::string fmt5 = fmt::format("{:g}", par5);
    ASSERT_EQ(fmt5, "5 F 1 10");

    std::string fmt6 = fmt::format("{:g}", par6);
    ASSERT_EQ(fmt6, "6.1 : 1e-06 10.1");
}*/

TEST(TestsParam, FormatterDefault)
{
    std::string fmt1 = fmt::format("{}", par1);
    ASSERT_EQ(fmt1, "0");

    std::string fmt2 = fmt::format("{}", par2);
    ASSERT_EQ(fmt2, "0");

    std::string fmt3 = fmt::format("{}", par3);
    ASSERT_EQ(fmt3, "3 f");

    std::string fmt4 = fmt::format("{}", par4);
    ASSERT_EQ(fmt4, "4");

    std::string fmt5 = fmt::format("{}", par5);
    ASSERT_EQ(fmt5, "5 F 1 10");

    std::string fmt6 = fmt::format("{}", par6);
    ASSERT_EQ(fmt6, "6.1 : 1e-06 10.1");
}

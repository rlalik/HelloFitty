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

TEST(tests_HistogramFitParam, basic)
{
    HistogramFitParams hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    ASSERT_STREQ(hfp1.hist_name, "h1");
    ASSERT_STREQ(hfp1.sig_string, "gaus(0)");
    ASSERT_STREQ(hfp1.bkg_string, "expo(3)");

    ASSERT_EQ(hfp1.range_l, 1);
    ASSERT_EQ(hfp1.range_u, 10);

    ASSERT_EQ(hfp1.rebin, 0);
    ASSERT_EQ(hfp1.fit_disabled, false);
}

TEST(tests_FitterFactory, prefix_suffix_test)
{
    std::string f1 = "pref1_*";
    std::string f2 = "p_*_suff1";

    std::string tn1 = "test_name";
    std::string tn2 = "replaced";

    FitterFactory ff1;
    FitterFactory ff2;

    ASSERT_STREQ("pref1_test_name", ff1.format_name(tn1, f1));
    ASSERT_STREQ("pref1_replaced", ff1.format_name(tn2, f1));

    ASSERT_STREQ("p_test_name_suff1", ff2.format_name(tn1, f2));
    ASSERT_STREQ("p_replaced_suff1", ff2.format_name(tn2, f2));
}

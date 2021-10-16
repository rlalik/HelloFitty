#include <gtest/gtest.h>

#include "FitterFactory.h"

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

TEST(tests_HistogramFitParam, cloning)
{
    HistogramFitParams hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    auto hfp2 = hfp1.clone("h2");

    ASSERT_STRNE(hfp1.hist_name, hfp2->hist_name);
    ASSERT_STREQ(hfp2->hist_name, "h2");
    ASSERT_STREQ(hfp1.sig_string, hfp2->sig_string);
    ASSERT_STREQ(hfp1.bkg_string, hfp2->bkg_string);

    ASSERT_EQ(hfp1.range_l, hfp2->range_l);
    ASSERT_EQ(hfp1.range_u, hfp2->range_u);

    ASSERT_EQ(hfp1.rebin, hfp2->rebin);
    ASSERT_EQ(hfp1.fit_disabled, hfp2->fit_disabled);
}

TEST(tests_HistogramFitParam, backups)
{
    HistogramFitParams hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    ParamValue p1;
    auto p2 = ParamValue();
    auto p3 = ParamValue(3, ParamValue::FitMode::Fixed);
    auto p4 = ParamValue(4, 1, 10, ParamValue::FitMode::Free);

    hfp1.setParam(0, p1);
    hfp1.setParam(1, p2);
    hfp1.setParam(2, p3);
    hfp1.setParam(3, p4);

    hfp1.push();

    ASSERT_EQ(hfp1.pars[0].val, 0);
    ASSERT_EQ(hfp1.pars[1].val, 0);
    ASSERT_EQ(hfp1.pars[2].val, 3);
    ASSERT_EQ(hfp1.pars[3].val, 4);

    hfp1.pars[0].val = 10;
    hfp1.pars[1].val = 20;
    hfp1.pars[2].val = 30;
    hfp1.pars[3].val = 40;

    hfp1.apply();

    ASSERT_EQ(hfp1.pars[0].val, 0);
    ASSERT_EQ(hfp1.pars[1].val, 0);
    ASSERT_EQ(hfp1.pars[2].val, 3);
    ASSERT_EQ(hfp1.pars[3].val, 4);

    hfp1.pars[0].val = 10;
    hfp1.pars[1].val = 20;
    hfp1.pars[2].val = 30;
    hfp1.pars[3].val = 40;

    hfp1.pop();

    ASSERT_EQ(hfp1.pars[0].val, 0);
    ASSERT_EQ(hfp1.pars[1].val, 0);
    ASSERT_EQ(hfp1.pars[2].val, 3);
    ASSERT_EQ(hfp1.pars[3].val, 4);

    hfp1.pars[0].val = 10;
    hfp1.pars[1].val = 20;
    hfp1.pars[2].val = 30;
    hfp1.pars[3].val = 40;

    hfp1.pop();

    ASSERT_EQ(hfp1.pars[0].val, 10);
    ASSERT_EQ(hfp1.pars[1].val, 20);
    ASSERT_EQ(hfp1.pars[2].val, 30);
    ASSERT_EQ(hfp1.pars[3].val, 40);
}

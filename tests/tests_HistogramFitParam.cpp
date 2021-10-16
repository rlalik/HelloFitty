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

TEST(tests_HistogramFitParam, parsing_line)
{
    auto hfp0 =
        HistogramFitParams::parseLineEntry("hist_1 gaus(0) 0  0  1 10  1  2 : 1 3  3 F 2 5  4 f");

    ASSERT_FALSE(hfp0.get());

    auto hfp1 = HistogramFitParams::parseLineEntry(
        "hist_1 gaus(0) pol0(3)  0  1 10  1  2 : 1 3  3 F 2 5  4 f");

    ASSERT_STREQ(hfp1->hist_name, "hist_1");
    ASSERT_STREQ(hfp1->sig_string, "gaus(0)");
    ASSERT_STREQ(hfp1->bkg_string, "pol0(3)");

    ASSERT_EQ(hfp1->range_l, 1);
    ASSERT_EQ(hfp1->range_u, 10);

    ASSERT_EQ(hfp1->rebin, 0);
    ASSERT_EQ(hfp1->fit_disabled, false);

    ASSERT_EQ(hfp1->pars.size(), 4);

    ASSERT_EQ(hfp1->pars[0].val, 1);
    ASSERT_EQ(hfp1->pars[0].l, 0);
    ASSERT_EQ(hfp1->pars[0].u, 0);
    ASSERT_EQ(hfp1->pars[0].mode, ParamValue::FitMode::Free);
    ASSERT_EQ(hfp1->pars[0].has_limits, false);

    ASSERT_EQ(hfp1->pars[1].val, 2);
    ASSERT_EQ(hfp1->pars[1].l, 1);
    ASSERT_EQ(hfp1->pars[1].u, 3);
    ASSERT_EQ(hfp1->pars[1].mode, ParamValue::FitMode::Free);
    ASSERT_EQ(hfp1->pars[1].has_limits, true);

    ASSERT_EQ(hfp1->pars[2].val, 3);
    ASSERT_EQ(hfp1->pars[2].l, 2);
    ASSERT_EQ(hfp1->pars[2].u, 5);
    ASSERT_EQ(hfp1->pars[2].mode, ParamValue::FitMode::Fixed);
    ASSERT_EQ(hfp1->pars[2].has_limits, true);

    ASSERT_EQ(hfp1->pars[3].val, 4);
    ASSERT_EQ(hfp1->pars[3].l, 0);
    ASSERT_EQ(hfp1->pars[3].u, 0);
    ASSERT_EQ(hfp1->pars[3].mode, ParamValue::FitMode::Fixed);
    ASSERT_EQ(hfp1->pars[3].has_limits, false);

    hfp1->init();
    hfp1->print();

    auto export1 = hfp1->exportEntry();
    ASSERT_STREQ(export1, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp2 = HistogramFitParams::parseLineEntry(
        "hist_1 gaus(0) pol0(3)  1  1 10  1  2 : 1 3  3 F 2 5  4 f");

    ASSERT_STREQ(hfp2->hist_name, "hist_1");
    ASSERT_STREQ(hfp2->sig_string, "gaus(0)");
    ASSERT_STREQ(hfp2->bkg_string, "pol0(3)");

    ASSERT_EQ(hfp2->range_l, 1);
    ASSERT_EQ(hfp2->range_u, 10);

    ASSERT_EQ(hfp2->rebin, 0);
    ASSERT_EQ(hfp2->fit_disabled, false);

    ASSERT_EQ(hfp2->pars.size(), 4);

    ASSERT_EQ(hfp2->pars[0].val, 1);
    ASSERT_EQ(hfp2->pars[0].l, 0);
    ASSERT_EQ(hfp2->pars[0].u, 0);
    ASSERT_EQ(hfp2->pars[0].mode, ParamValue::FitMode::Free);
    ASSERT_EQ(hfp2->pars[0].has_limits, false);

    ASSERT_EQ(hfp2->pars[1].val, 2);
    ASSERT_EQ(hfp2->pars[1].l, 1);
    ASSERT_EQ(hfp2->pars[1].u, 3);
    ASSERT_EQ(hfp2->pars[1].mode, ParamValue::FitMode::Free);
    ASSERT_EQ(hfp2->pars[1].has_limits, true);

    ASSERT_EQ(hfp2->pars[2].val, 3);
    ASSERT_EQ(hfp2->pars[2].l, 2);
    ASSERT_EQ(hfp2->pars[2].u, 5);
    ASSERT_EQ(hfp2->pars[2].mode, ParamValue::FitMode::Fixed);
    ASSERT_EQ(hfp2->pars[2].has_limits, true);

    ASSERT_EQ(hfp2->pars[3].val, 4);
    ASSERT_EQ(hfp2->pars[3].l, 0);
    ASSERT_EQ(hfp2->pars[3].u, 0);
    ASSERT_EQ(hfp2->pars[3].mode, ParamValue::FitMode::Fixed);
    ASSERT_EQ(hfp2->pars[3].has_limits, false);

    hfp2->init();
    hfp2->print();

    auto export2 = hfp2->exportEntry();
    ASSERT_STREQ(export2, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp3 = HistogramFitParams::parseLineEntry("hist_1 gaus(0) pol0(3)  1  1 10");

    ASSERT_STREQ(hfp3->hist_name, "hist_1");
    ASSERT_STREQ(hfp3->sig_string, "gaus(0)");
    ASSERT_STREQ(hfp3->bkg_string, "pol0(3)");

    ASSERT_EQ(hfp3->range_l, 1);
    ASSERT_EQ(hfp3->range_u, 10);

    ASSERT_EQ(hfp3->rebin, 0);
    ASSERT_EQ(hfp3->fit_disabled, false);

    ASSERT_EQ(hfp3->pars.size(), 4);

    ASSERT_EQ(hfp3->pars[0].val, 0);
    ASSERT_EQ(hfp3->pars[0].l, 0);
    ASSERT_EQ(hfp3->pars[0].u, 0);
    ASSERT_EQ(hfp3->pars[0].mode, ParamValue::FitMode::Free);
    ASSERT_EQ(hfp3->pars[0].has_limits, false);

    ASSERT_EQ(hfp3->pars[1].val, 0);
    ASSERT_EQ(hfp3->pars[1].l, 0);
    ASSERT_EQ(hfp3->pars[1].u, 0);
    ASSERT_EQ(hfp3->pars[1].mode, ParamValue::FitMode::Free);
    ASSERT_EQ(hfp3->pars[1].has_limits, false);

    ASSERT_EQ(hfp3->pars[2].val, 0);
    ASSERT_EQ(hfp3->pars[2].l, 0);
    ASSERT_EQ(hfp3->pars[2].u, 0);
    ASSERT_EQ(hfp3->pars[2].mode, ParamValue::FitMode::Free);
    ASSERT_EQ(hfp3->pars[2].has_limits, false);

    ASSERT_EQ(hfp3->pars[3].val, 0);
    ASSERT_EQ(hfp3->pars[3].l, 0);
    ASSERT_EQ(hfp3->pars[3].u, 0);
    ASSERT_EQ(hfp3->pars[3].mode, ParamValue::FitMode::Free);
    ASSERT_EQ(hfp3->pars[3].has_limits, false);

    hfp3->init();
    hfp3->print();

    auto export3 = hfp3->exportEntry();
    ASSERT_STREQ(export3, " hist_1\tgaus(0) pol0(3) 0 1 10 0 0 0 0");

    auto hfp4 = HistogramFitParams::parseLineEntry("hist_1 gaus(0) pol0(3)  1  1");

    ASSERT_FALSE(hfp4.get());
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

    hfp1.drop();
}

#include <gtest/gtest.h>

#include "FitterFactory.h"

using FF::HistogramFit;

TEST(tests_HistogramFit, basic)
{
    HistogramFit hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    ASSERT_STREQ(hfp1.getName(), "h1");
    ASSERT_STREQ(hfp1.getSigString(), "gaus(0)");
    ASSERT_STREQ(hfp1.getBkgString(), "expo(3)");

    ASSERT_EQ(hfp1.getFitRangeL(), 1);
    ASSERT_EQ(hfp1.getFitRangeU(), 10);

    ASSERT_EQ(hfp1.getFlagRebin(), 0);
    ASSERT_EQ(hfp1.getFlagDisabled(), false);
}

TEST(tests_HistogramFit, parsing_line)
{
    auto hfp0 = FF::parseLineEntry("hist_1 gaus(0) 0  0  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_FALSE(hfp0.get());

    auto hfp1 = FF::parseLineEntry("hist_1 gaus(0) pol0(3)  0  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_STREQ(hfp1->getName(), "hist_1");
    ASSERT_STREQ(hfp1->getSigString(), "gaus(0)");
    ASSERT_STREQ(hfp1->getBkgString(), "pol0(3)");

    ASSERT_EQ(hfp1->getFitRangeL(), 1);
    ASSERT_EQ(hfp1->getFitRangeU(), 10);

    ASSERT_EQ(hfp1->getFlagRebin(), 0);
    ASSERT_EQ(hfp1->getFlagDisabled(), false);

    ASSERT_EQ(hfp1->getParamsNumber(), 4);

    ASSERT_EQ(hfp1->getParam(0).val, 1);
    ASSERT_EQ(hfp1->getParam(0).l, 0);
    ASSERT_EQ(hfp1->getParam(0).u, 0);
    ASSERT_EQ(hfp1->getParam(0).mode, FF::Param::FitMode::Free);
    ASSERT_EQ(hfp1->getParam(0).has_limits, false);

    ASSERT_EQ(hfp1->getParam(1).val, 2);
    ASSERT_EQ(hfp1->getParam(1).l, 1);
    ASSERT_EQ(hfp1->getParam(1).u, 3);
    ASSERT_EQ(hfp1->getParam(1).mode, FF::Param::FitMode::Free);
    ASSERT_EQ(hfp1->getParam(1).has_limits, true);

    ASSERT_EQ(hfp1->getParam(2).val, 3);
    ASSERT_EQ(hfp1->getParam(2).l, 2);
    ASSERT_EQ(hfp1->getParam(2).u, 5);
    ASSERT_EQ(hfp1->getParam(2).mode, FF::Param::FitMode::Fixed);
    ASSERT_EQ(hfp1->getParam(2).has_limits, true);

    ASSERT_EQ(hfp1->getParam(3).val, 4);
    ASSERT_EQ(hfp1->getParam(3).l, 0);
    ASSERT_EQ(hfp1->getParam(3).u, 0);
    ASSERT_EQ(hfp1->getParam(3).mode, FF::Param::FitMode::Fixed);
    ASSERT_EQ(hfp1->getParam(3).has_limits, false);

    hfp1->init();
    hfp1->print();

    auto export1 = hfp1->exportEntry();
    ASSERT_STREQ(export1, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp2 = FF::parseLineEntry("hist_1 gaus(0) pol0(3)  1  1 10  1  2 : 1 3  3 F 2 5  4 f", 0);

    ASSERT_STREQ(hfp2->getName(), "hist_1");
    ASSERT_STREQ(hfp2->getSigString(), "gaus(0)");
    ASSERT_STREQ(hfp2->getBkgString(), "pol0(3)");

    ASSERT_EQ(hfp2->getFitRangeL(), 1);
    ASSERT_EQ(hfp2->getFitRangeU(), 10);

    ASSERT_EQ(hfp2->getFlagRebin(), 0);
    ASSERT_EQ(hfp2->getFlagDisabled(), false);

    ASSERT_EQ(hfp2->getParamsNumber(), 4);

    ASSERT_EQ(hfp2->getParam(0).val, 1);
    ASSERT_EQ(hfp2->getParam(0).l, 0);
    ASSERT_EQ(hfp2->getParam(0).u, 0);
    ASSERT_EQ(hfp2->getParam(0).mode, FF::Param::FitMode::Free);
    ASSERT_EQ(hfp2->getParam(0).has_limits, false);

    ASSERT_EQ(hfp2->getParam(1).val, 2);
    ASSERT_EQ(hfp2->getParam(1).l, 1);
    ASSERT_EQ(hfp2->getParam(1).u, 3);
    ASSERT_EQ(hfp2->getParam(1).mode, FF::Param::FitMode::Free);
    ASSERT_EQ(hfp2->getParam(1).has_limits, true);

    ASSERT_EQ(hfp2->getParam(2).val, 3);
    ASSERT_EQ(hfp2->getParam(2).l, 2);
    ASSERT_EQ(hfp2->getParam(2).u, 5);
    ASSERT_EQ(hfp2->getParam(2).mode, FF::Param::FitMode::Fixed);
    ASSERT_EQ(hfp2->getParam(2).has_limits, true);

    ASSERT_EQ(hfp2->getParam(3).val, 4);
    ASSERT_EQ(hfp2->getParam(3).l, 0);
    ASSERT_EQ(hfp2->getParam(3).u, 0);
    ASSERT_EQ(hfp2->getParam(3).mode, FF::Param::FitMode::Fixed);
    ASSERT_EQ(hfp2->getParam(3).has_limits, false);

    hfp2->init();
    hfp2->print();

    auto export2 = hfp2->exportEntry();
    ASSERT_STREQ(export2, " hist_1\tgaus(0) pol0(3) 0 1 10 1 2 : 1 3 3 F 2 5 4 f");

    auto hfp3 = FF::parseLineEntry("hist_1 gaus(0) pol0(3)  1  1 10", 0);

    ASSERT_STREQ(hfp3->getName(), "hist_1");
    ASSERT_STREQ(hfp3->getSigString(), "gaus(0)");
    ASSERT_STREQ(hfp3->getBkgString(), "pol0(3)");

    ASSERT_EQ(hfp3->getFitRangeL(), 1);
    ASSERT_EQ(hfp3->getFitRangeU(), 10);

    ASSERT_EQ(hfp3->getFlagRebin(), 0);
    ASSERT_EQ(hfp3->getFlagDisabled(), false);

    ASSERT_EQ(hfp3->getParamsNumber(), 4);

    ASSERT_EQ(hfp3->getParam(0).val, 0);
    ASSERT_EQ(hfp3->getParam(0).l, 0);
    ASSERT_EQ(hfp3->getParam(0).u, 0);
    ASSERT_EQ(hfp3->getParam(0).mode, FF::Param::FitMode::Free);
    ASSERT_EQ(hfp3->getParam(0).has_limits, false);

    ASSERT_EQ(hfp3->getParam(1).val, 0);
    ASSERT_EQ(hfp3->getParam(1).l, 0);
    ASSERT_EQ(hfp3->getParam(1).u, 0);
    ASSERT_EQ(hfp3->getParam(1).mode, FF::Param::FitMode::Free);
    ASSERT_EQ(hfp3->getParam(1).has_limits, false);

    ASSERT_EQ(hfp3->getParam(2).val, 0);
    ASSERT_EQ(hfp3->getParam(2).l, 0);
    ASSERT_EQ(hfp3->getParam(2).u, 0);
    ASSERT_EQ(hfp3->getParam(2).mode, FF::Param::FitMode::Free);
    ASSERT_EQ(hfp3->getParam(2).has_limits, false);

    ASSERT_EQ(hfp3->getParam(3).val, 0);
    ASSERT_EQ(hfp3->getParam(3).l, 0);
    ASSERT_EQ(hfp3->getParam(3).u, 0);
    ASSERT_EQ(hfp3->getParam(3).mode, FF::Param::FitMode::Free);
    ASSERT_EQ(hfp3->getParam(3).has_limits, false);

    hfp3->init();
    hfp3->print();

    auto export3 = hfp3->exportEntry();
    ASSERT_STREQ(export3, " hist_1\tgaus(0) pol0(3) 0 1 10 0 0 0 0");

    auto hfp4 = FF::parseLineEntry("hist_1 gaus(0) pol0(3)  1  1", 0);

    ASSERT_FALSE(hfp4.get());
}

TEST(tests_HistogramFit, cloning)
{
    HistogramFit hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    auto hfp2 = hfp1.clone("h2");

    ASSERT_STRNE(hfp1.getName(), hfp2->getName());
    ASSERT_STREQ(hfp2->getName(), "h2");
    ASSERT_STREQ(hfp1.getSigString(), hfp2->getSigString());
    ASSERT_STREQ(hfp1.getBkgString(), hfp2->getBkgString());

    ASSERT_EQ(hfp1.getFitRangeL(), hfp2->getFitRangeL());
    ASSERT_EQ(hfp1.getFitRangeU(), hfp2->getFitRangeU());

    ASSERT_EQ(hfp1.getFlagRebin(), hfp2->getFlagRebin());
    ASSERT_EQ(hfp1.getFlagDisabled(), hfp2->getFlagDisabled());

    hfp1.push();
    hfp1.clear();
}

TEST(tests_HistogramFit, backups)
{
    HistogramFit hfp1("h1", "gaus(0)", "expo(3)", 1, 10);

    FF::Param p1;
    auto p2 = FF::Param();
    auto p3 = FF::Param(3, FF::Param::FitMode::Fixed);
    auto p4 = FF::Param(4, 1, 10, FF::Param::FitMode::Free);

    ASSERT_EQ(hfp1.getParamsNumber(), 5);

    hfp1.setParam(0, p1);
    hfp1.setParam(1, p2);
    hfp1.setParam(2, p3);
    hfp1.setParam(3, p4);

    // try {
    //     hfp1.setParam(5, p4);
    // } catch (...) {
    //     ASSERT_NE(hfp1.getParamsNumber(), 5);
    // }

    hfp1.push();

    ASSERT_EQ(hfp1.getParam(0).val, 0);
    ASSERT_EQ(hfp1.getParam(1).val, 0);
    ASSERT_EQ(hfp1.getParam(2).val, 3);
    ASSERT_EQ(hfp1.getParam(3).val, 4);

    hfp1.updateParam(0, 10);
    hfp1.updateParam(1, 20);
    hfp1.updateParam(2, 30);
    hfp1.updateParam(3, 40);

    hfp1.apply();

    ASSERT_EQ(hfp1.getParam(0).val, 0);
    ASSERT_EQ(hfp1.getParam(1).val, 0);
    ASSERT_EQ(hfp1.getParam(2).val, 3);
    ASSERT_EQ(hfp1.getParam(3).val, 4);

    hfp1.updateParam(0, 10);
    hfp1.updateParam(1, 20);
    hfp1.updateParam(2, 30);
    hfp1.updateParam(3, 40);

    hfp1.pop();

    ASSERT_EQ(hfp1.getParam(0).val, 0);
    ASSERT_EQ(hfp1.getParam(1).val, 0);
    ASSERT_EQ(hfp1.getParam(2).val, 3);
    ASSERT_EQ(hfp1.getParam(3).val, 4);

    hfp1.updateParam(0, 10);
    hfp1.updateParam(1, 20);
    hfp1.updateParam(2, 30);
    hfp1.updateParam(3, 40);

    hfp1.pop();

    ASSERT_EQ(hfp1.getParam(0).val, 10);
    ASSERT_EQ(hfp1.getParam(1).val, 20);
    ASSERT_EQ(hfp1.getParam(2).val, 30);
    ASSERT_EQ(hfp1.getParam(3).val, 40);

    hfp1.drop();
}

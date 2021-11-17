#include <gtest/gtest.h>

#include "FitterFactory.h"

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

TEST(tests_FitterFactory, insert_parameters)
{
    FitterFactory ff;

    auto hf1 = std::make_unique<HistogramFit>("name1", "1", "0", 0, 1);

    auto o1 = ff.findFit("name1");
    ASSERT_EQ(o1, nullptr);

    ff.insertParameters(std::move(hf1));

    auto o2 = ff.findFit("name1");
    ASSERT_NE(o2, nullptr);
}

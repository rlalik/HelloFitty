#include <gtest/gtest.h>

#include "FitterFactory.h"

TEST(tests_FitterFactory, basic_tests)
{
    FitterFactory ff;

    std::string p1 = "pref1_";
    std::string s1 = "_suff1";

    std::string tn1 = "test_name";
    std::string tn2 = p1 + tn1 + s1;
    std::string tn3 = "replaced";
    std::string tn4 = p1 + tn3 + s1;

    ff.setPrefix(p1);
    ff.setSuffix(s1);

    ASSERT_EQ(tn1, ff.format_name(tn1));
    ASSERT_EQ(tn2, ff.format_name(tn2));

    ff.setPrefixManipulator(FitterFactory::PSFIX::APPEND);
    ff.setSuffixManipulator(FitterFactory::PSFIX::IGNORE);
    ASSERT_EQ(p1 + tn1, ff.format_name(tn1));

    ff.setPrefixManipulator(FitterFactory::PSFIX::IGNORE);
    ff.setSuffixManipulator(FitterFactory::PSFIX::APPEND);
    ASSERT_EQ(tn1 + s1, ff.format_name(tn1));

    ff.setPrefixManipulator(FitterFactory::PSFIX::APPEND);
    ff.setSuffixManipulator(FitterFactory::PSFIX::APPEND);
    ASSERT_EQ(p1 + tn1 + s1, ff.format_name(tn1));

    ff.setPrefixManipulator(FitterFactory::PSFIX::SUBSTRACT);
    ff.setSuffixManipulator(FitterFactory::PSFIX::IGNORE);
    ASSERT_EQ(tn1 + s1, ff.format_name(tn2));

    ff.setPrefixManipulator(FitterFactory::PSFIX::IGNORE);
    ff.setSuffixManipulator(FitterFactory::PSFIX::SUBSTRACT);
    ASSERT_EQ(p1 + tn1, ff.format_name(tn2));

    ff.setPrefixManipulator(FitterFactory::PSFIX::SUBSTRACT);
    ff.setSuffixManipulator(FitterFactory::PSFIX::SUBSTRACT);
    ASSERT_EQ(tn1, ff.format_name(tn2));

    ff.setReplacement(tn1, tn3);
    ASSERT_EQ(tn3, ff.format_name(tn2));
}

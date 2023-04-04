#include <gtest/gtest.h>

#include "FitterFactory.h"

#if __cplusplus < 201402L
template <typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
using std::make_unique;
#endif

TEST(tests_FitterFactory, prefix_suffix_test)
{
    std::string f1 = "pref1_*";
    std::string f2 = "p_*_suff1";

    std::string tn1 = "test_name";
    std::string tn2 = "replaced";

    FF::FitterFactory ff1;
    FF::FitterFactory ff2;

    ASSERT_STREQ("pref1_test_name", FF::Tools::format_name(tn1, f1));
    ASSERT_STREQ("pref1_replaced", FF::Tools::format_name(tn2, f1));

    ASSERT_STREQ("p_test_name_suff1", FF::Tools::format_name(tn1, f2));
    ASSERT_STREQ("p_replaced_suff1", FF::Tools::format_name(tn2, f2));
}

TEST(tests_FitterFactory, insert_parameters)
{
    FF::FitterFactory ff;

    auto o1 = ff.findFit("name1");
    ASSERT_EQ(o1, nullptr);

    auto hf1 = make_unique<FF::HistogramFit>("name1", "1", "0", 0, 1);
    ff.insertParameters(std::move(hf1));

    auto o2 = ff.findFit("name1");
    ASSERT_NE(o2, nullptr);
}

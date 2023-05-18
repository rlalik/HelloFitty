#include "gtest/gtest.h"

#include "fitemall.hpp"

#include <TString.h>

#include <memory>
#include <string>
#include <utility>

#if __cplusplus < 201402L
template <typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
using std::make_unique;
#endif

TEST(tests_fitter, prefix_suffix_test)
{
    std::string f1 = "pref1_*";
    std::string f2 = "p_*_suff1";

    std::string tn1 = "test_name";
    std::string tn2 = "replaced";

    fea::fitter ff1;
    fea::fitter ff2;

    ASSERT_STREQ("pref1_test_name", fea::tools::format_name(tn1, f1));
    ASSERT_STREQ("pref1_replaced", fea::tools::format_name(tn2, f1));

    ASSERT_STREQ("p_test_name_suff1", fea::tools::format_name(tn1, f2));
    ASSERT_STREQ("p_replaced_suff1", fea::tools::format_name(tn2, f2));
}

TEST(tests_fitter, insert_parameters)
{
    fea::fitter ff;

    auto o1 = ff.find_fit("name1");
    ASSERT_EQ(o1, nullptr);

    auto hf1 = make_unique<fea::histogram_fit>("name1", "1", "0", 0, 1);
    ff.insert_parameters(std::move(hf1));

    auto o2 = ff.find_fit("name1");
    ASSERT_NE(o2, nullptr);
}

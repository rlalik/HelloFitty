#include <gtest/gtest.h>

#include <hellofitty.hpp>

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

TEST(TestsFitter, PrefixSuffixTest)
{
    const std::string pat1 = "pref1_*";
    const std::string pat2 = "p_*_suff1";

    const std::string tn1 = "test_name";
    const std::string tn2 = "replaced";

    ASSERT_STREQ("pref1_test_name", hf::tools::format_name(tn1, pat1));
    ASSERT_STREQ("pref1_replaced", hf::tools::format_name(tn2, pat1));

    ASSERT_STREQ("p_test_name_suff1", hf::tools::format_name(tn1, pat2));
    ASSERT_STREQ("p_replaced_suff1", hf::tools::format_name(tn2, pat2));
}

TEST(TestsFitter, InsertParameters)
{
    hf::fitter fitter;

    const auto fit1 = fitter.find_fit("name1");
    ASSERT_EQ(fit1, nullptr);

    auto hf1 = make_unique<hf::fit_entry>("name1", 0, 1);
    hf1->add_function("1");
    hf1->add_function("0");
    fitter.insert_parameters(std::move(hf1));

    const auto fit2 = fitter.find_fit("name1");
    ASSERT_NE(fit2, nullptr);
}

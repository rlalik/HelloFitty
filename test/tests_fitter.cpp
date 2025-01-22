#include <gtest/gtest.h>

#include "hellofitty.hpp"

#include "details.hpp"

#include <TF1.h>
#include <TH1.h>

#include <memory>
#include <string>
#include <utility>

auto make_hist() { return std::make_unique<TH1I>("h_foo", "foo", 10, 0, 10); }

TEST(TestsFitter, PrefixSuffixTest)
{
    const std::string pat1 = "pref1_*";
    const std::string pat2 = "p_*_suff1";

    const std::string tn1 = "test_name";
    const std::string tn2 = "replaced";

    ASSERT_STREQ("pref1_test_name", hf::tools::format_name(tn1, pat1).c_str());
    ASSERT_STREQ("pref1_replaced", hf::tools::format_name(tn2, pat1).c_str());

    ASSERT_STREQ("p_test_name_suff1", hf::tools::format_name(tn1, pat2).c_str());
    ASSERT_STREQ("p_replaced_suff1", hf::tools::format_name(tn2, pat2).c_str());
}

TEST(TestsFitter, InsertParameters)
{
    hf::fitter fitter;

    const auto fit1 = fitter.find_fit("name1");
    ASSERT_EQ(fit1, nullptr);

    auto hf1 = hf::entry(0, 1);
    hf1.add_function("1");
    hf1.add_function("0");
    fitter.insert_parameter("name1", hf1);

    const auto fit2 = fitter.find_fit("name1");
    ASSERT_NE(fit2, nullptr);

    fitter.clear();
}

TEST(TestsFitter, FitFinding)
{
    hf::fitter fitter;
    auto h_foo = make_hist();

    const auto fit1 = fitter.find_fit(h_foo.get());
    ASSERT_EQ(fit1, nullptr);

    const auto fit2 = fitter.find_fit(h_foo.get());
    ASSERT_EQ(fit2, nullptr);

    fitter.clear();
}

TEST(TestsFitter, FittingHist)
{
    hf::fitter fitter;
    auto h_foo = make_hist();

    hf::entry hfp_defaults(0, 10);
    EXPECT_THROW(fitter.fit(h_foo.get(), "", "", &hfp_defaults), std::logic_error);
}

TEST(TestsFitter, FittingHistBadHFP)
{
    hf::fitter fitter;
    auto h_foo = make_hist();

    hf::entry hfp_defaults(0, 10);
    ASSERT_EQ(hfp_defaults.add_function("gaus(0)"), 0);

    ASSERT_FALSE(fitter.fit(h_foo.get(), "", "").first);

    // test fitting without function
    ASSERT_FALSE(fitter.fit(h_foo.get(), "", "", &hfp_defaults).first);
}

TEST(TestsFitter, FittingHistGoodHFP)
{
    hf::fitter fitter;
    hf::entry hfp_defaults(0, 10);
    auto h_foo = make_hist();

    auto fgaus = std::make_unique<TF1>("f_gaus", "gaus", 0, 10);
    fgaus->SetParameters(1, 5, 1);
    h_foo->FillRandom("f_gaus");

    ASSERT_EQ(hfp_defaults.add_function("gaus(0)"), 0);

    // test fitting empty range
    // fitter.find_fit(h_foo->GetName())->set_fit_range(0, 10);
    ASSERT_TRUE(fitter.fit(h_foo.get(), "", "", &hfp_defaults).first);

    fitter.clear();
}

TEST(TestsFitter, FittingGraph)
{
    hf::fitter fitter;

    // TH1I* h_foo = new TH1I("h_foo", "foo", 10, 0, 10);
    // // EXPECT_THROW(fitter.fit(h_foo, "", ""), std::logic_error);
    //
    // hf::entry hfp_defaults(1, 10);
    // EXPECT_THROW(fitter.fit(h_foo, "", "", &hfp_defaults), std::logic_error);
    //
    // ASSERT_EQ(hfp_defaults.add_function("gaus(0)"), 0);
    //
    // const auto fit1 = fitter.fit(h_foo, "", "");
    // ASSERT_FALSE(fit1.first);
    //
    // const auto fit2 = fitter.fit(h_foo, "", "", &hfp_defaults);
    // ASSERT_TRUE(fit2.first);
    //
    // fitter.clear();
    // delete h_foo;
}

TEST(TestsFitter, NameDecorator)
{
    hf::fitter fitter;

    auto hfp_foo = hf::entry(1, 10);
    ASSERT_EQ(hfp_foo.add_function("gaus(0)"), 0);

    fitter.insert_parameter("h_foo", hfp_foo);

    fitter.set_name_decorator("*_foo");

    TH1I* h_foo = new TH1I("h", "foo", 10, 0, 10);

    const auto entry1 = fitter.find_fit(h_foo);
    ASSERT_NE(entry1, nullptr);

    fitter.clear();
    delete h_foo;
}

TEST(TestsFitter, FunctionDecorator)
{
    hf::fitter fitter;

    fitter.set_function_decorator("*_test");

    fitter.clear();
}

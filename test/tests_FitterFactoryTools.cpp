#include <gtest/gtest.h>

#include "FitterFactory.h"

#include "ffconfig.h"

TEST(tests_FitterFactoryTools, source_select)
{
    auto true_in = tests_src_path + "test_input.txt";
    auto true_out = tests_src_path + "test_output.txt";

    auto fake_in = tests_src_path + "fake_input.txt";
    auto fake_out = tests_src_path + "fake_output.txt";

    auto newer_one = build_path + "ffconfig.h";

    ASSERT_EQ(FF::Tools::selectSource(fake_in.c_str(), fake_out.c_str()),
              FF::Tools::SelectedSource::None);

    ASSERT_EQ(FF::Tools::selectSource(true_in.c_str(), fake_out.c_str()),
              FF::Tools::SelectedSource::OnlyReference);

    ASSERT_EQ(FF::Tools::selectSource(fake_in.c_str(), true_out.c_str()),
              FF::Tools::SelectedSource::OnlyAuxiliary);

    ASSERT_EQ(FF::Tools::selectSource(true_in.c_str(), newer_one.c_str()),
              FF::Tools::SelectedSource::Auxiliary);

    ASSERT_EQ(FF::Tools::selectSource(newer_one.c_str(), true_out.c_str()),
              FF::Tools::SelectedSource::Reference);
}

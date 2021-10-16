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

    ASSERT_EQ(FitterFactoryTools::selectSource(fake_in.c_str(), fake_out.c_str()),
              FitterFactoryTools::SelectedSource::None);

    ASSERT_EQ(FitterFactoryTools::selectSource(true_in.c_str(), fake_out.c_str()),
              FitterFactoryTools::SelectedSource::OnlyReference);

    ASSERT_EQ(FitterFactoryTools::selectSource(fake_in.c_str(), true_out.c_str()),
              FitterFactoryTools::SelectedSource::OnlyAuxilary);

    ASSERT_EQ(FitterFactoryTools::selectSource(true_in.c_str(), newer_one.c_str()),
              FitterFactoryTools::SelectedSource::Auxilary);

    ASSERT_EQ(FitterFactoryTools::selectSource(newer_one.c_str(), true_out.c_str()),
              FitterFactoryTools::SelectedSource::Reference);
}

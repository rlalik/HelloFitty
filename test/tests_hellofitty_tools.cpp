#include <gtest/gtest.h>

#include "hellofitty.hpp"

#include "hellofitty_config.h"

TEST(tests_HelloKitty_Tools, source_select)
{
    auto true_in = tests_src_path + "test_input.txt";
    auto true_out = tests_src_path + "test_output.txt";

    auto fake_in = tests_src_path + "fake_input.txt";
    auto fake_out = tests_src_path + "fake_output.txt";

    auto newer_one = build_path + "hellofitty_config.h";

    ASSERT_EQ(hf::tools::select_source(fake_in.c_str(), fake_out.c_str()),
              hf::tools::selected_source::none);

    ASSERT_EQ(hf::tools::select_source(true_in.c_str(), fake_out.c_str()),
              hf::tools::selected_source::only_reference);

    ASSERT_EQ(hf::tools::select_source(fake_in.c_str(), true_out.c_str()),
              hf::tools::selected_source::only_auxiliary);

    ASSERT_EQ(hf::tools::select_source(true_in.c_str(), newer_one.c_str()),
              hf::tools::selected_source::auxiliary);

    ASSERT_EQ(hf::tools::select_source(newer_one.c_str(), true_out.c_str()),
              hf::tools::selected_source::reference);
}

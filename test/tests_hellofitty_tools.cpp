#include <gtest/gtest.h>

#include "hellofitty.hpp"
#include "hellofitty_config.h"

TEST(TestsTools, FormatDetection)
{
    ASSERT_EQ(hf::tools::detect_format("hist_1 gaus(0) 0  0  1 10  1  2 : 1 3  3 F 2 5"), hf::format_version::v1);

    ASSERT_EQ(hf::tools::detect_format("hist_1 1 10 0 gaus(0) | 1  2 : 1 3  3 F 2 5"), hf::format_version::v2);
}

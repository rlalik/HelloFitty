cmake_minimum_required(VERSION 3.14)

project(HelloFittyTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(HelloFitty REQUIRED)
  enable_testing()
endif()

set(INSTALL_GTEST OFF)
find_or_fetch_package(GTest https://github.com/google/googletest GIT_TAG release-1.11.0)

if(TARGET GTest::gtest_main)
  # new cmake provided
  set(GTEST_TRG GTest::gtest_main)
else()
  # old cmake and pc files
  set(GTEST_TRG gtest gtest_main)
endif()

# ---- Tests ----

set(tests_SRCS tests_param.cpp
               tests_entry.cpp
               tests_parser_v1.cpp
               tests_parser_v2.cpp
               tests_fitter.cpp
               tests_hellofitty_tools.cpp)

add_executable(gtests ${tests_SRCS})
target_link_libraries(gtests
    PRIVATE
        HelloFitty::HelloFitty
        ROOT::Core
        ${GTEST_TRG}
        ${FMT_TARGET}
)

if(ENABLE_ADVANCE_TOOLS)
  target_code_coverage(gtests ALL)
endif()

gtest_discover_tests(gtests)

# ---- End-of-file commands ----

add_folders(Test)

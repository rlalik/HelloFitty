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
find_or_fetch_package(GTest https://github.com/google/googletest GIT_TAG v1.15.2)

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
        GTest::gtest_main
        ${FMT_TARGET}
)

include(GoogleTest)
gtest_discover_tests(gtests)

# ---- End-of-file commands ----

add_folders(Test)

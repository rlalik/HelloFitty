include(../cmake/find_or_fetch_package.cmake)

set(INSTALL_GTEST OFF)
find_or_fetch_package(GTest https://github.com/google/googletest GIT_TAG release-1.11.0)

set(tests_SRCS tests_param.cpp
               tests_fit_entry.cpp
               tests_parser_v1.cpp
               tests_parser_v2.cpp
               tests_hellofitty.cpp
               tests_hellofitty_tools.cpp)

add_executable(gtests ${tests_SRCS})

target_include_directories(gtests PRIVATE ${CMAKE_BINARY_DIR})

if(TARGET GTest::gtest_main)
  # new cmake provided
  set(GTEST_TRG GTest::gtest_main)
else()
  # old cmake and pc files
  set(GTEST_TRG gtest gtest_main)
endif()

target_link_libraries(gtests PRIVATE HelloFitty::HelloFitty ROOT::Core ${GTEST_TRG} ${FMT_TARGET})
if(ENABLE_ADVANCE_TOOLS)
  target_code_coverage(gtests ALL)
endif()

gtest_discover_tests(gtests)

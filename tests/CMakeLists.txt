find_package(GTest REQUIRED)

set(tests_SRCS
    tests_ParamValue.cpp
    tests_HistogramFit.cpp
    tests_FitterFactory.cpp
    tests_FitterFactoryTools.cpp
)

add_executable(
    gtests
    ${tests_SRCS}
)

target_include_directories(gtests PRIVATE ${CMAKE_BINARY_DIR})

if (TARGET GTest::gtest_main)
    # new cmake provided
    set(GTEST_TRG GTest::gtest_main)
else()
    # old cmake and pc files
    set(GTEST_TRG gtest gtest_main)
endif()

target_link_libraries(
    gtests
    PRIVATE
        FitterFactory
        ROOT::Core
        ${GTEST_TRG}
)
#target_code_coverage(gtests ALL)

gtest_discover_tests(gtests)

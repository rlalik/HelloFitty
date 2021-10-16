find_package(GTest REQUIRED)

#configure_file(tests_config.h.in tests_config.h)

set(tests_SRCS
    tests_FitterFactory.cpp
)

add_executable(
    gtests
    ${tests_SRCS}
)

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

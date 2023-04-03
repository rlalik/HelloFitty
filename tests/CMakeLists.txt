include(FetchContent)
FetchContent_Declare(cmake-tools
    GIT_REPOSITORY https://github.com/rlalik/cmake_tools
    GIT_TAG        master # from main after sanitizer updates
)
FetchContent_MakeAvailable(cmake-tools)

list(APPEND CMAKE_MODULE_PATH
        ${cmake-tools_SOURCE_DIR}
)

include(find_or_fetch_package)
set(INSTALL_GTEST OFF)
find_or_fetch_package(GTest https://github.com/google/googletest GIT_TAG main)

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

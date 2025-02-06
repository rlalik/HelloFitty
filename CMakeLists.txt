cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    HelloFitty
    VERSION 0.21.0
    DESCRIPTION "Hello Fitty - a versatile histogram fitting tool for ROOT-based projects."
    HOMEPAGE_URL "https://github.com/rlalik/HelloFitty"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Dependencies ----

include(cmake/find_or_fetch_package.cmake)

# find ROOT
find_package(ROOT QUIET REQUIRED COMPONENTS Core Hist)

# FMT
find_or_fetch_package(fmt https://github.com/fmtlib/fmt GIT_TAG 11.1.3 VERSION 11.1.3)
if (fmt_FETCHED)
  set(FMT_TARGET $<BUILD_INTERFACE:fmt::fmt-header-only>)
else()
  set(FMT_TARGET fmt::fmt)
endif()

# ---- Declare library ----

configure_file(config.h.in inc/hellofitty_config.h)

add_library(
    HelloFitty
    source/hellofitty.cpp
    source/draw_opts.cpp
    source/param.cpp
    source/entry.cpp
    source/fitter.cpp
    source/parser_v1.cpp
    source/parser_v2.cpp
)
add_library(HelloFitty::HelloFitty ALIAS HelloFitty)

target_link_libraries(HelloFitty
    PUBLIC ROOT::Core ROOT::Hist
    PRIVATE ${FMT_TARGET}
)

include(GenerateExportHeader)
generate_export_header(
    HelloFitty
    BASE_NAME HelloFitty
    EXPORT_FILE_NAME export/HelloFitty/hellofitty_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(HelloFitty PUBLIC HELLOFITTY_STATIC_DEFINE)
endif()

set_target_properties(
    HelloFitty PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME HelloFitty
    OUTPUT_NAME HelloFitty
)

target_include_directories(
    HelloFitty ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>"
)

target_include_directories(
    HelloFitty SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/inc>"
)

option(BUILD_DICTIONARY "Build ROOT dictionary." OFF)
if (BUILD_DICTIONARY)
    # cmake-format: off
    root_generate_dictionary(G__HelloFitty_cc hellofitty.hpp
        MODULE HelloFitty
        LINKDEF inc/LinkDef.h)
    # cmake-format: on
endif()

# target_compile_features(HelloFitty PUBLIC cxx_std_11)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${HelloFitty_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT HelloFitty_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of HelloFitty"
  )
endif()

include(cmake/dev-mode.cmake)

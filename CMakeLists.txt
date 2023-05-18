cmake_minimum_required(VERSION 3.12)

include(cmake/prelude.cmake)

project(
    FitterFactory
    VERSION 0.9.4
    DESCRIPTION "Versatile histogram fitting tool for ROOT-based projects."
    HOMEPAGE_URL "https://github.com/rlalik/FitterFactory"
    LANGUAGES CXX
)


include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# find ROOT
#list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT QUIET REQUIRED COMPONENTS Core Hist)

# ---- Declare library ----

add_library(
    FitterFactory
    source/FitterFactory.cpp
    source/parser_v1.cpp
)
add_library(RT::FitterFactory ALIAS FitterFactory)

target_link_libraries(FitterFactory PUBLIC ROOT::Core ROOT::Hist)

include(GenerateExportHeader)
generate_export_header(
    FitterFactory
    BASE_NAME FitterFactory
    EXPORT_FILE_NAME export/FitterFactory/FitterFactory_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(FitterFactory PUBLIC FITTERFACTORY_STATIC_DEFINE)
endif()

set_target_properties(
    FitterFactory PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME FitterFactory
    OUTPUT_NAME FitterFactory
)

target_include_directories(
    FitterFactory ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>"
)

target_include_directories(
    FitterFactory SYSTEM
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

option(BUILD_DICTIONARY "Build ROOT dictionary." OFF)
if (BUILD_DICTIONARY)
  # cmake-format: off
  root_generate_dictionary(G__${PROJECT_NAME}_cc FitterFactory.h
    MODULE FitterFactory
    LINKDEF inc/LinkDef.h)
  # cmake-format: on
endif()

target_compile_features(FitterFactory PUBLIC cxx_std_11)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${FitterFactory_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT FitterFactory_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of FitterFactory"
  )
endif()

include(cmake/dev-mode.cmake)

# Export the package for use from the build-tree (this registers the build-tree
# with a global CMake-registry)
export(PACKAGE ${PROJECT_NAME})

configure_file(config.h.in ffconfig.h)

# messages
message(
  STATUS
    "<<< Configuration >>>
Project:        ${PROJECT_NAME}

Architecture:   ${CMAKE_CXX_LIBRARY_ARCHITECTURE}
Build type      ${CMAKE_BUILD_TYPE}
Install path    ${CMAKE_INSTALL_PREFIX}

Compiler:
C++             ${CMAKE_CXX_COMPILER}

Linker:
Ld              ${CMAKE_LINKER}

Compiler flags:
C++             ${CMAKE_CXX_FLAGS}

Linker flags:
Executable      ${CMAKE_EXE_LINKER_FLAGS}
Module          ${CMAKE_MODULE_LINKER_FLAGS}
Shared          ${CMAKE_SHARED_LINKER_FLAGS}\n")

foreach(p LIB BIN INCLUDE)
  message(STATUS "CMAKE_INSTALL_${p}DIR: ${CMAKE_INSTALL_${p}DIR}")
endforeach()

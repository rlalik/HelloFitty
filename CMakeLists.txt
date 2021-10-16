cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(FitterFactory
    VERSION 0.9.0
    LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME})

### conditional modules
option(ENABLE_ADVANCE_CHECKS "Enables advanced checks like sanitizers" OFF)
include(FetchContent)
FetchContent_Declare(cmake-scripts
    GIT_REPOSITORY https://github.com/StableCoder/cmake-scripts.git
    GIT_TAG        774a07ece3945b4f72e3ddc11646ab7cc9a349e2 # from main after sanitizer updates
)

if(ENABLE_ADVANCE_CHECKS)
    FetchContent_MakeAvailable(cmake-scripts)
    list(APPEND CMAKE_MODULE_PATH
        ${cmake-scripts_SOURCE_DIR}
    )
    message(STATUS "${cmake-scripts_SOURCE_DIR}")
    include(sanitizers)
endif()

##### find ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT QUIET REQUIRED COMPONENTS Core Hist)

add_library(${CMAKE_PROJECT_NAME} SHARED
    src/FitterFactory.cxx
)
add_library(RT::${CMAKE_PROJECT_NAME} ALIAS ${CMAKE_PROJECT_NAME})

target_include_directories(${CMAKE_PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PUBLIC
        ROOT::Core
        ROOT::Hist
)

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
        PUBLIC_HEADER
            "inc/FitterFactory.h"
)

ROOT_GENERATE_DICTIONARY(G__${CMAKE_PROJECT_NAME}_cc
    FitterFactory.h
    MODULE ${CMAKE_PROJECT_NAME}
    LINKDEF LinkDef.h
)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE ${CMAKE_PROJECT_NAME})

# Install the export set for use with the install-tree
install(TARGETS ${CMAKE_PROJECT_NAME}
    EXPORT ${CMAKE_PROJECT_NAME}Targets
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(EXPORT ${CMAKE_PROJECT_NAME}Targets
    FILE ${CMAKE_PROJECT_NAME}Targets.cmake
    NAMESPACE RT::
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR} COMPONENT dev)

include(CMakePackageConfigHelpers)
configure_package_config_file(Config.cmake.in ${CMAKE_PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_BINDIR)

write_basic_package_version_file(${CMAKE_PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

configure_file(config.h.in ffconfig.h)

option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(ENABLE_TESTING "Build tests" ON)
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# messages
MESSAGE(STATUS "<<< Configuration >>>
Project:        ${CMAKE_PROJECT_NAME}

Architecture:   ${CMAKE_LIBRARY_ARCHITECTURE}
Build type      ${CMAKE_BUILD_TYPE}
Install path    ${CMAKE_INSTALL_PREFIX}

Compiler:
C               ${CMAKE_C_COMPILER}
C++             ${CMAKE_CXX_COMPILER}

Linker:
Ld              ${CMAKE_LINKER}

Compiler flags:
C               ${CMAKE_C_FLAGS}
C++             ${CMAKE_CXX_FLAGS}

Linker flags:
Executable      ${CMAKE_EXE_LINKER_FLAGS}
Module          ${CMAKE_MODULE_LINKER_FLAGS}
Shared          ${CMAKE_SHARED_LINKER_FLAGS}\n")

foreach(p LIB BIN INCLUDE CMAKE)
    message(STATUS "CMAKE_INSTALL_${p}DIR: ${CMAKE_INSTALL_${p}DIR}")
endforeach()

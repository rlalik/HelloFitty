cmake_minimum_required(VERSION 2.6)
project(FitterFactory C CXX)
set(CAPS_PROJECT_NAME FITTERFACTORY)

if(WIN32 AND NOT CYGWIN)
	set(DEF_CMAKE_INSTALL_CMAKEDIR CMake)
else()
	set(DEF_CMAKE_INSTALL_CMAKEDIR lib/CMake/${CMAKE_PROJECT_NAME})
endif()
set(CMAKE_INSTALL_CMAKEDIR ${DEF_CMAKE_INSTALL_CMAKEDIR} CACHE PATH "Installation directory for CMake files")
set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "object code libraries (lib)")
set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "object code libraries (lib)")
set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "object code libraries (lib)")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT QUIET
	REQUIRED COMPONENTS Core  RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix Physics MathCore Thread
	OPTIONAL_COMPONENTS Cling Cint)

# The version number.
set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (VERSION_PATCH 1)
set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

##### find programs
include(${ROOT_USE_FILE})

include_directories(${ROOT_INCLUDE_DIR} inc)
link_directories(${ROOT_LIBRARIES_DIR})

ROOT_STANDARD_LIBRARY_PACKAGE(FitterFactory DEPENDENCIES ${ROOT_LIBRARIES})

add_subdirectory(examples)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE ${CMAKE_PROJECT_NAME})


include(CMakePackageConfigHelpers)
configure_package_config_file(Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
															INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
															PATH_VARS
																CMAKE_INSTALL_INCLUDEDIR
																CMAKE_INSTALL_LIBDIR
																CMAKE_INSTALL_BINDIR)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
																VERSION ${PROJECT_VERSION}
																COMPATIBILITY SameMajorVersion)
                                 
install(FILES
					${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
					${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

# Add all targets to the build-tree export set
export(TARGETS FitterFactory FILE "${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Targets.cmake")

# Install the export set for use with the install-tree
install(TARGETS FitterFactory
	EXPORT ${CMAKE_PROJECT_NAME}Targets
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	COMPONENT shlib COMPONENT dev)

install(EXPORT ${CMAKE_PROJECT_NAME}Targets DESTINATION "${CMAKE_INSTALL_CMAKEDIR}" COMPONENT dev)

# messages
MESSAGE(STATUS "<<< Configuration >>>
Project:	${CMAKE_PROJECT_NAME}

Build type	${CMAKE_BUILD_TYPE}
Install path	${CMAKE_INSTALL_PREFIX}

Compiler:
C		${CMAKE_C_COMPILER}
C++		${CMAKE_CXX_COMPILER}

Linker:
Ld		${CMAKE_LINKER}

Compiler flags:
C		${CMAKE_C_FLAGS}
C++		${CMAKE_CXX_FLAGS}

Linker flags:
Executable      ${CMAKE_EXE_LINKER_FLAGS}
Module          ${CMAKE_MODULE_LINKER_FLAGS}
Shared          ${CMAKE_SHARED_LINKER_FLAGS}\n")

foreach(p LIB BIN INCLUDE CMAKE)
	message(STATUS "CMAKE_INSTALL_${p}DIR: ${CMAKE_INSTALL_${p}DIR}")
endforeach()
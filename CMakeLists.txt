cmake_minimum_required(VERSION 2.6)
project(FitterFactory)
set(PRONAME FitterFactory)

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
set (library_VERSION_MAJOR 0)
set (library_VERSION_MINOR 1)
set (library_VERSION_PATCH 1)

set (VERSION ${library_VERSION_MAJOR}.${library_VERSION_MINOR}.${library_VERSION_PATCH})

##### find programs
include(${ROOT_USE_FILE})

include_directories(${ROOT_INCLUDE_DIR} inc)
link_directories(${ROOT_LIBRARIES_DIR})

ROOT_STANDARD_LIBRARY_PACKAGE(${PRONAME} DEPENDENCIES ${ROOT_LIBRARIES})

add_executable(example examples/example.cxx)
target_link_libraries(example ${ROOT_LIBRARIES} FitterFactory)

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

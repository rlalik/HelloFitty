include(FetchContent)

#[[
The main task of this is to fetch external tool if one is not available in the system.
It has three main modes:
 * FETCH - force to fetch external tool
 * FIND - force to use system tool
 * AUTO - check the system, and if not available; default mode

Usage

include(find_or_fetch_package)
find_or_fetch_package(name url [VERSION version] [GIT_TAG optional_tag] [FETCH|FIND|AUTO] [EXPORT_OPTION])

Find mode
---------

If FIND mode is selected, the find_package(${name} ${VERSION} QUITE) is called.

Fetch mode
----------

If FETCH mode is selected, FetchContent_Declare will be called. If GIT_TAG is set, it will be used otherwise VERSION is use. If neitherGIT_TAG or VERSION is provided, will result in fatal error in FETCH|AUTO mode.

Auto mode
---------

It first try to find local installation (FIND mode) and if unsuccessful, call FETCH mode.

The GIT_TAG should be provided if the VERSION cannot be used as a tag name, or another branch or commit hash is required.

The macro will register option with name ${PROJECT_NAME}_BUILTIN_${upper_case_name} (upper case of name) which can be controlled from command line, e.g.

If variable USE_BUILTIN_xxx (where xxx is the capitalized name) is set, it has priority over macro call modes. This variable should take value of the these three modes: FETCH, FIND or AUTO, e.g.:

cmake -DUSE_BUILTIN_xxx=FETCH

If EXPORT_OPTION is used it creates CACHE entry of USE_BUILTIN_xxx.

]]
macro(find_or_fetch_package name url)
  set(options FIND FETCH AUTO EXPORT_OPTION)
  set(args VERSION GIT_TAG)
  set(args_mult "")
  cmake_parse_arguments(ARG "${options}" "${args}" "${args_mult}" ${ARGN})

  set(${name}_FETCHED 0)

  string(TOUPPER ${name} ucName)
  string(TOLOWER ${name} lcName)

  # git tag (branch, tag, etc) may be different than version name, even due to
  # semantics
  if(ARG_GIT_TAG)
    set(GIT_TAG ${ARG_GIT_TAG})
  else(ARG_VERSION)
    set(GIT_TAG ${ARG_VERSION})
  endif()

  if(USE_BUILTIN_${ucName})
    set(DEFAULT ${USE_BUILTIN_${ucName}})
  elseif(ARG_FIND)
    set(DEFAULT FIND)
  elseif(ARG_FETCH)
    set(DEFAULT FETCH)
  else()
    set(DEFAULT AUTO)
  endif()

  if (ARG_EXPORT_OPTION)
    set(USE_BUILTIN_${ucName}
      ${DEFAULT}
      CACHE STRING "Fetch or find ${name}")
    set_property(CACHE USE_BUILTIN_${ucName} PROPERTY STRINGS AUTO;FIND;FETCH)
  endif()

  if(DEFAULT STREQUAL AUTO)
    find_package(${name} ${ARG_VERSION} QUIET)
    if(NOT ${name}_FOUND)
      set(USE_BUILTIN_${ucName} TRUE)
    endif()
  elseif(DEFAULT STREQUAL FIND) # a true value (such as ON) was used
    set(USE_BUILTIN_${ucName} FALSE)
    find_package(${name} ${version} REQUIRED)
  elseif(DEFAULT STREQUAL FETCH) # a false value (such as OFF) was used
    set(USE_BUILTIN_${ucName} TRUE)
  else()
    message(FATAL_ERROR "No FETCH/FIND/AUTO mode selected. Use ${USE_BUILTIN_${ucName}} or set proper mode")
  endif()

  if(USE_BUILTIN_${ucName})

    # We need to know either version or tag to fetch proper branch
    if ("${GIT_TAG}" STREQUAL "")
      message(FATAL_ERROR "Please provide either VERSION or GIT_TAG for ${url} to fetch.")
    endif()

    message(STATUS "Fetching ${name} from URL ${url} TAG ${GIT_TAG} ... Please wait...")
    FetchContent_Declare(
      ${name}
      GIT_REPOSITORY ${url}
      GIT_TAG ${GIT_TAG})

    # FetchContent_MakeAvailable requires CMake-3.14 or newer
    if(CMAKE_VERSION VERSION_LESS 3.14)
      FetchContent_GetProperties(${name})
      if(NOT ${lcName}_POPULATED)
        FetchContent_Populate(${name})
        if(EXISTS ${${lcName}_SOURCE_DIR}/CMakeLists.txt)
          add_subdirectory(${${lcName}_SOURCE_DIR} ${${lcName}_BINARY_DIR})
        endif()
      endif()
    else()
      FetchContent_MakeAvailable(${name})
    endif()

    set(${name}_FETCHED 1)
  else()
    message(STATUS "Uses system-provided ${name} ${ARG_VERSION}")
  endif()

endmacro()

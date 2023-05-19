include(FetchContent)

macro(find_or_fetch_package name url)
  set(options FIND FETCH AUTO EXPORT_OPTION)
  set(args VERSION GIT_TAG)
  set(args_mult "")
  cmake_parse_arguments(ARG "${options}" "${args}" "${args_mult}" ${ARGN})

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

  else()
    message(STATUS "Uses system-provided ${name} ${ARG_VERSION}")
  endif()

endmacro()

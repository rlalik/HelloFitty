cmake_minimum_required(VERSION 3.14)

project(HelloFittyExamples CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(HelloFitty REQUIRED)
endif()

add_custom_target(run-examples)

function(add_example NAME)
  add_executable("${NAME}" "${NAME}.cpp")
  target_link_libraries("${NAME}" PRIVATE HelloFitty::HelloFitty ${FMT_TARGET})
  target_compile_features("${NAME}" PRIVATE cxx_std_17)
  add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-examples "run_${NAME}")
endfunction()

add_example(example1)
add_example(example2)

if(RootTools_FOUND)
  # target_link_libraries(example2 RT::RootTools)
endif(RootTools_FOUND)

add_folders(Example)

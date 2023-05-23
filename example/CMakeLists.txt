# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_INSTALL_PREFIX}/lib/CMake)
# set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX}/lib/CMake)

# include(find_or_fetch_package)
# find_or_fetch_package(RootTools https://github.com/rlalik/RootTools GIT_TAG
                      # master)

# set(HAS_ROOTTOOLS ${RootTools_FOUND})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(example1 example1.cpp)
target_include_directories(example1 PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(example1 HelloFitty::HelloFitty)

add_executable(example2 example2.cpp)
target_include_directories(example2 PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(example2 HelloFitty::HelloFitty)

if(RootTools_FOUND)
  # target_link_libraries(example2 RT::RootTools)
endif(RootTools_FOUND)

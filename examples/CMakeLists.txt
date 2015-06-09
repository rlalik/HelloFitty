set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_INSTALL_PREFIX}/lib/CMake)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX}/lib/CMake)

find_package(RootTools)
set(HAS_ROOTTOOLS ${RootTools_FOUND})

configure_file(config.h.in ffconfig.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${ROOTTOOLS_INCLUDE_DIR})
link_directories(${ROOTTOOLS_LIBRARY_DIR})

add_executable(example1 example1.cxx)
target_link_libraries(example1 ${ROOT_LIBRARIES} FitterFactory)

add_executable(example2 example2.cxx)
target_link_libraries(example2 ${ROOT_LIBRARIES} FitterFactory ${ROOTTOOLS_TARGET})

if (RootTools_FOUND)
	target_link_libraries(example2 RootTools)
endif (RootTools_FOUND)

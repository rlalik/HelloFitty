if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/hellofitty-${PROJECT_VERSION}"
      CACHE PATH ""
  )
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package HelloFitty)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT HelloFitty_Development
)

install(
    TARGETS HelloFitty
    EXPORT HelloFittyTargets
    RUNTIME #
    COMPONENT HelloFitty_Runtime
    LIBRARY #
    COMPONENT HelloFitty_Runtime
    NAMELINK_COMPONENT HelloFitty_Development
    ARCHIVE #
    COMPONENT HelloFitty_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    HelloFitty_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(HelloFitty_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${HelloFitty_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT HelloFitty_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${HelloFitty_INSTALL_CMAKEDIR}"
    COMPONENT HelloFitty_Development
)

install(
    EXPORT HelloFittyTargets
    NAMESPACE HelloFitty::
    DESTINATION "${HelloFitty_INSTALL_CMAKEDIR}"
    COMPONENT HelloFitty_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()

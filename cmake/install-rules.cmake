if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/FitterFactory-${PROJECT_VERSION}"
      CACHE PATH ""
  )
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package FitterFactory)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT FitterFactory_Development
)

install(
    TARGETS FitterFactory
    EXPORT FitterFactoryTargets
    RUNTIME #
    COMPONENT FitterFactory_Runtime
    LIBRARY #
    COMPONENT FitterFactory_Runtime
    NAMELINK_COMPONENT FitterFactory_Development
    ARCHIVE #
    COMPONENT FitterFactory_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    FitterFactory_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(FitterFactory_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${FitterFactory_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT FitterFactory_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${FitterFactory_INSTALL_CMAKEDIR}"
    COMPONENT FitterFactory_Development
)

install(
    EXPORT FitterFactoryTargets
    NAMESPACE FF::
    DESTINATION "${FitterFactory_INSTALL_CMAKEDIR}"
    COMPONENT FitterFactory_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()

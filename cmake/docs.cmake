# ---- Dependencies ----

set(extract_timestamps "")
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
  set(extract_timestamps DOWNLOAD_EXTRACT_TIMESTAMP YES)
endif()

include(FetchContent)
FetchContent_Declare(
    mcss
    GIT_REPOSITORY https://github.com/mosra/m.css/
    GIT_TAG master
    SOURCE_DIR "${PROJECT_BINARY_DIR}/mcss"
    UPDATE_DISCONNECTED YES
    ${extract_timestamps}
)
FetchContent_MakeAvailable(mcss)

find_package(Python3 3 REQUIRED)

# ---- Declare documentation target ----

set(
    DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs"
    CACHE PATH "Path for the generated Doxygen documentation"
)

set(working_dir "${PROJECT_BINARY_DIR}/docs")

foreach(file IN ITEMS Doxyfile conf.py)
  configure_file("docs/${file}.in" "${working_dir}/${file}" @ONLY)
endforeach()

set(mcss_script "${mcss_SOURCE_DIR}/documentation/doxygen.py")
set(config "${working_dir}/conf.py")

# Fix libgs absolute path
find_library(LIBGS gs)
if (LIBGS-NOTFOUND)
  message(FATAL_ERROR "libgs is not found")
else()
  message(STATUS "libgs found in ${LIBGS}")
  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/mcss/plugins/latex2svg.py_fix
    COMMAND sed
      -E \"s|\(libgs_absolute\ =\ \).+|\\1\\\"${LIBGS}\\\"|g\"
      -i ${PROJECT_BINARY_DIR}/mcss/plugins/latex2svg.py
    DEPENDS ${PROJECT_BINARY_DIR}/mcss/plugins/latex2svg.py
  )
endif()
# Fix end

add_custom_target(
    docs
    COMMAND "${CMAKE_COMMAND}" -E remove_directory
    "${DOXYGEN_OUTPUT_DIRECTORY}/html"
    "${DOXYGEN_OUTPUT_DIRECTORY}/xml"
    COMMAND "${Python3_EXECUTABLE}" "${mcss_script}" "${config}"
    COMMENT "Building documentation using Doxygen and m.css"
    WORKING_DIRECTORY "${working_dir}"
    DEPENDS ${PROJECT_BINARY_DIR}/mcss/plugins/latex2svg.py_fix
    VERBATIM
)

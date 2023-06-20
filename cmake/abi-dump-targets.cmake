add_custom_target(
    abi-dump
    COMMAND abi-dumper
    libHelloFitty.so
    -o ABI-${PROJECT_VERSION_MAJOR}
    -lver ${PROJECT_VERSION_MAJOR}
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMENT "Dump ABI"
    VERBATIM
)

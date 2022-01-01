execute_process(COMMAND ares-package "${CPACK_TEMPORARY_DIRECTORY}"
    -e include
    -e cmake
    -e "libmbedtls[.].*"
    )
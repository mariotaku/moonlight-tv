execute_process(COMMAND ares-package "${CPACK_TEMPORARY_DIRECTORY}" -o "${CPACK_PACKAGE_DIRECTORY}"
        -e include
        -e cmake
        -e "libmbedtls[.].*"
        -e "lib/static"
        -e "lib/pkgconfig"
        --force-arch "${CPACK_WEBOS_PACKAGE_ARCH}"
        COMMAND_ERROR_IS_FATAL ANY
)

find_program(GEN_MANIFEST webosbrew-gen-manifest)
if (NOT GEN_MANIFEST)
    message(STATUS "Manifest generator not found, skipping manifest generation")
    return()
endif ()

execute_process(COMMAND ${GEN_MANIFEST} -p "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.ipk"
        -o "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_NAME}.manifest.json"
        -i "https://github.com/mariotaku/moonlight-tv/raw/main/deploy/webos/icon.png"
        -l "https://github.com/mariotaku/moonlight-tv"
        COMMAND_ERROR_IS_FATAL ANY
)
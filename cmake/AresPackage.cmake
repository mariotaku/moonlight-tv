execute_process(COMMAND ares-package "${CPACK_TEMPORARY_DIRECTORY}" -o "${CPACK_PACKAGE_DIRECTORY}"
        -e include
        -e cmake
        -e "libmbedtls[.].*"
        -e "lib/static"
        COMMAND_ERROR_IS_FATAL ANY
        )

execute_process(COMMAND webosbrew-gen-manifest -p "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.ipk"
        -o "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_NAME}.manifest.json"
        -i "https://github.com/mariotaku/moonlight-tv/raw/main/deploy/webos/icon.png"
        -l "https://github.com/mariotaku/moonlight-tv"
        COMMAND_ERROR_IS_FATAL ANY
        )
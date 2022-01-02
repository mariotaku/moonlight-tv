execute_process(COMMAND ares-package "${CPACK_TEMPORARY_DIRECTORY}" -o "${CPACK_PACKAGE_DIRECTORY}"
        -e include
        -e cmake
        -e "libmbedtls[.].*"
        )

execute_process(COMMAND npm run webos-gen-manifest -- -a "${CPACK_TEMPORARY_DIRECTORY}/appinfo.json"
        -p "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.ipk"
        -o "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_NAME}.manifest.json"
        -i "https://github.com/mariotaku/moonlight-tv/raw/main/deploy/webos/icon.png"
        -l "https://github.com/mariotaku/moonlight-tv"
        )
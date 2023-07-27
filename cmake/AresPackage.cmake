if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    file(GLOB_RECURSE PKG_BINARIES RELATIVE "${CPACK_TEMPORARY_DIRECTORY}" "${CPACK_TEMPORARY_DIRECTORY}/bin/*"
            "${CPACK_TEMPORARY_DIRECTORY}/lib/*.so*")

    execute_process(COMMAND ${CMAKE_COMMAND} -E chdir "${CPACK_TEMPORARY_DIRECTORY}" tar cfvz
            "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}_dbgsym.tar.gz" -- ${PKG_BINARIES})

    foreach (PKG_BINARY ${PKG_BINARIES})
        execute_process(COMMAND "$ENV{STRIP}" ${PKG_BINARY} WORKING_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}"
                COMMAND_ERROR_IS_FATAL ANY)
    endforeach ()
endif ()

execute_process(COMMAND ares-package "${CPACK_TEMPORARY_DIRECTORY}" -o "${CPACK_PACKAGE_DIRECTORY}"
        -e include
        -e cmake
        -e "libmbedtls[.].*"
        -e "lib/static"
        )

execute_process(COMMAND webosbrew-gen-manifest -p "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.ipk"
        -o "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_NAME}.manifest.json"
        -i "https://github.com/mariotaku/moonlight-tv/raw/main/deploy/webos/icon.png"
        -l "https://github.com/mariotaku/moonlight-tv"
        )
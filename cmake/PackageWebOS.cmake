find_program(AWK awk REQUIRED)

# Copy manifest
configure_file(deploy/webos/appinfo.json ./appinfo.json @ONLY)

# Copy all files under deploy/webos/ to package root
install(DIRECTORY deploy/webos/ DESTINATION . PATTERN ".*" EXCLUDE PATTERN "*.in" EXCLUDE PATTERN "appinfo.json" EXCLUDE)
install(FILES "${CMAKE_BINARY_DIR}/appinfo.json" DESTINATION .)

# Generate translations
foreach (I18N_LOCALE ${I18N_LOCALES})
    string(REPLACE "-" "/" I18N_JSON_DIR "resources/${I18N_LOCALE}")
    install(CODE "file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${I18N_JSON_DIR}\")"
            CODE "execute_process(COMMAND ${AWK} -f scripts/webos/po2json.awk i18n/${I18N_LOCALE}/messages.po
                OUTPUT_FILE \"\${CMAKE_INSTALL_PREFIX}/${I18N_JSON_DIR}/cstrings.json\"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND_ERROR_IS_FATAL ANY)")
endforeach ()

# Generation gamepad mapping
install(CODE "file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/assets\")"
        CODE "execute_process(COMMAND scripts/webos/gen_gamecontrollerdb.sh
            OUTPUT_FILE \"\${CMAKE_INSTALL_PREFIX}/assets/gamecontrollerdb.txt\"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND_ERROR_IS_FATAL ANY)")

# Fake library for cURL ABI issue
add_dependencies(moonlight commons-curl-abi-fix)
install(TARGETS commons-curl-abi-fix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP)

set(CPACK_PACKAGE_NAME "${WEBOS_APPINFO_ID}")
set(CPACK_GENERATOR "External")
set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/AresPackage.cmake")
set(CPACK_EXTERNAL_ENABLE_STAGING TRUE)
set(CPACK_MONOLITHIC_INSTALL TRUE)
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_$ENV{ARCH}")
# Will use all cores on CMake 3.20+
set(CPACK_THREADS 0)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CPACK_STRIP_FILES TRUE)
endif ()

add_custom_target(webos-package-moonlight COMMAND cpack DEPENDS moonlight)


# Used by webos-install-moonlight
set_target_properties(moonlight PROPERTIES
        WEBOS_PACKAGE_TARGET webos-package-moonlight
        WEBOS_PACKAGE_PATH "${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.ipk"
        WEBOS_APPINFO_ID "${WEBOS_APPINFO_ID}"
        )

include(CPack)
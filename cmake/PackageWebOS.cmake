# Copy manifest
configure_file(deploy/webos/appinfo.json.in ./appinfo.json @ONLY)

# Copy all files under deploy/webos/ to package root
install(DIRECTORY deploy/webos/ DESTINATION . PATTERN ".*" EXCLUDE PATTERN "*.in" EXCLUDE)
install(FILES "${CMAKE_BINARY_DIR}/appinfo.json" DESTINATION .)
install(CODE "execute_process(COMMAND npm run webos-gen-i18n -- -o \"\${CMAKE_INSTALL_PREFIX}/resources\" ${I18N_LOCALES})")

add_custom_target(webos-generate-gamecontrollerdb
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/webos/gen_gamecontrollerdb.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

set(WEBOS_PACKAGE_FILENAME ${WEBOS_APPINFO_ID}_${PROJECT_VERSION}_$ENV{ARCH}.ipk)

set(CPACK_GENERATOR "External")
set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/AresPackage.cmake")
set(CPACK_EXTERNAL_ENABLE_STAGING TRUE)
set(CPACK_MONOLITHIC_INSTALL TRUE)

add_custom_target(webos-package-moonlight COMMAND cpack)

set_target_properties(moonlight PROPERTIES
        WEBOS_PACKAGE_TARGET webos-package-moonlight
        WEBOS_PACKAGE_PATH ${CMAKE_BINARY_DIR}/${WEBOS_PACKAGE_FILENAME}
        WEBOS_PACKAGE_FILENAME ${WEBOS_PACKAGE_FILENAME}
        WEBOS_APPINFO_ID ${WEBOS_APPINFO_ID}
        )

include(CPack)
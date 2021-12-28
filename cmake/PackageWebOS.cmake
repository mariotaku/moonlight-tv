# Copy all files under deploy/webos/ to package root
install(DIRECTORY deploy/webos/ DESTINATION ${CMAKE_INSTALL_WEBOS_PKGDIR} PATTERN ".*" EXCLUDE PATTERN "*.in" EXCLUDE)
# Copy manifest
configure_file(deploy/webos/appinfo.json.in ${CMAKE_INSTALL_WEBOS_PKGDIR}/appinfo.json @ONLY)
configure_file(cmake/CPackWebOS.cmake.in ${CMAKE_BINARY_DIR}/CPackWebOS.cmake @ONLY)

add_custom_target(webos-generate-gamecontrollerdb
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/webos/gen_gamecontrollerdb.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

set(WEBOS_PACKAGE_FILENAME ${WEBOS_APPINFO_ID}_${PROJECT_VERSION}_$ENV{ARCH}.ipk)

set(CPACK_GENERATOR "External")
set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_BINARY_DIR}/CPackWebOS.cmake")

add_custom_target(webos-package-moonlight COMMAND cpack)
add_dependencies(webos-package-moonlight moonlight)

set_target_properties(moonlight PROPERTIES
        WEBOS_PACKAGE_TARGET webos-package-moonlight
        WEBOS_PACKAGE_PATH ${CMAKE_BINARY_DIR}/${WEBOS_PACKAGE_FILENAME}
        WEBOS_PACKAGE_FILENAME ${WEBOS_PACKAGE_FILENAME}
        WEBOS_APPINFO_ID ${WEBOS_APPINFO_ID}
        )

include(CPack)
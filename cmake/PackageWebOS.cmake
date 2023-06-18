if ($ENV{Python3_ROOT_DIR})
    find_program(Python3_EXECUTABLE python3 HINTS "$ENV{Python3_ROOT_DIR}/bin" REQUIRED)
else ()
    set(Python3_FIND_VIRTUALENV FIRST)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif ()

# Copy manifest
configure_file(deploy/webos/appinfo.json ./appinfo.json @ONLY)

# Copy all files under deploy/webos/ to package root
install(DIRECTORY deploy/webos/ DESTINATION . PATTERN ".*" EXCLUDE PATTERN "*.in" EXCLUDE PATTERN "appinfo.json" EXCLUDE)
install(FILES "${CMAKE_BINARY_DIR}/appinfo.json" DESTINATION .)
install(CODE "execute_process(COMMAND ${Python3_EXECUTABLE} scripts/webos/gen_i18n.py -o \"\${CMAKE_INSTALL_PREFIX}/resources\" ${I18N_LOCALES} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})")

# Fake library for cURL ABI issue
add_dependencies(moonlight commons-curl-abi-fix)
install(TARGETS commons-curl-abi-fix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP)

add_custom_target(webos-generate-gamecontrollerdb
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/webos/gen_gamecontrollerdb.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

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
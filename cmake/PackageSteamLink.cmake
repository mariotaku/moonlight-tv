if (NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX moonlight-tv)
endif ()

# Copy all files under deploy/webos/ to package root
install(DIRECTORY deploy/steamlink/ DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS PATTERN ".*" EXCLUDE)

set(CPACK_PACKAGE_NAME "moonlight-tv")
set(CPACK_GENERATOR "ZIP")
set(CPACK_MONOLITHIC_INSTALL TRUE)
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_steamlink")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake/CleanupNameLink.cmake")
# Will use all cores on CMake 3.20+
set(CPACK_THREADS 0)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CPACK_STRIP_FILES TRUE)
endif ()

add_custom_target(steamlink-package-moonlight COMMAND cpack DEPENDS moonlight)

if (NOT ENV{CI})
    add_custom_target(steamlink-install-moonlight
            COMMAND scp "${CPACK_PACKAGE_FILE_NAME}.zip" steamlink:/tmp/
            COMMAND ssh steamlink 'unzip -o /tmp/${CPACK_PACKAGE_FILE_NAME}.zip -d /home/apps/'
            COMMAND ssh steamlink 'rm -f /tmp/${CPACK_PACKAGE_FILE_NAME}.zip'
            WORKING_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}
            DEPENDS steamlink-package-moonlight)
endif ()

include(CPack)
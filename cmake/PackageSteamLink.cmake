# Copy all files under deploy/webos/ to package root
install(DIRECTORY deploy/steamlink/ DESTINATION . USE_SOURCE_PERMISSIONS PATTERN ".*" EXCLUDE)

set(CPACK_PACKAGE_NAME "moonlight-tv")
set(CPACK_GENERATOR "TGZ")
set(CPACK_MONOLITHIC_INSTALL TRUE)
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_steamlink")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
# Will use all cores on CMake 3.20+
set(CPACK_THREADS 0)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CPACK_STRIP_FILES TRUE)
endif ()

add_custom_target(steamlink-package-moonlight COMMAND cpack DEPENDS moonlight)

if (NOT ENV{CI})
    add_custom_target(steamlink-install-moonlight
            COMMAND ssh steamlink 'mkdir -p /home/apps/moonlight-tv\; cd /home/apps/moonlight-tv\; tar zxvf -' < "${CPACK_PACKAGE_FILE_NAME}.tar.gz"
            WORKING_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}
            DEPENDS steamlink-package-moonlight)
endif ()

include(CPack)
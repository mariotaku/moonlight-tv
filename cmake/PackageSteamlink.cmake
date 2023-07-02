# Copy all files under deploy/webos/ to package root
install(DIRECTORY deploy/steamlink/ DESTINATION . USE_SOURCE_PERMISSIONS PATTERN ".*" EXCLUDE)

set(CPACK_PACKAGE_NAME "moonlight-tv")
set(CPACK_GENERATOR "TGZ")
set(CPACK_MONOLITHIC_INSTALL TRUE)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_steamlink")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
# Will use all cores on CMake 3.20+
set(CPACK_THREADS 0)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CPACK_STRIP_FILES TRUE)
endif ()

include(CPack)
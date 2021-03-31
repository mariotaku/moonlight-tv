cmake_minimum_required (VERSION 3.6)

unset(CMAKE_LIBRARY_OUTPUT_DIRECTORY)

add_subdirectory(stub)
target_compile_options(dile_vdec_direct PRIVATE -fPIC)
target_compile_options(helpers PRIVATE -fPIC)
target_compile_options(h264bitstream PRIVATE -fPIC)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/assets/lib)

pkg_check_modules(UMEDIA_API REQUIRED umedia_api_c)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(MEDIARESCALC REQUIRED media-resource-calculator)

add_library(moonlight-dile SHARED 
    dile_platform.c
    dile_video.c
    lunasynccall.c
    VideoOutputService.c
    ${CMAKE_SOURCE_DIR}/app/platform/webos/os_info.c
    ${CMAKE_SOURCE_DIR}/core/libgamestream/src/sps.c
)
target_include_directories(moonlight-dile PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/core/libgamestream/libgamestream
)
target_link_libraries(moonlight-dile moonlight-common-c h264bitstream
    dile_vdec_direct helpers
    ${GLIB_LIBRARIES} ${PBNJSON_LIBRARIES} ${LUNA_SERVICE_LIBRARIES} resource_mgr_client_c ${MEDIARESCALC_LIBRARIES})
target_include_directories(moonlight-dile SYSTEM PRIVATE
    ${GLIB_INCLUDE_DIRS} ${PBNJSON_INCLUDE_DIRS} ${LUNA_SERVICE_INCLUDE_DIRS} ${UMEDIA_API_INCLUDE_DIRS} ${MEDIARESCALC_INCLUDE_DIRS})

target_compile_options(moonlight-dile PRIVATE -fPIC)
target_compile_definitions(moonlight-dile PUBLIC HAVE_DILE)

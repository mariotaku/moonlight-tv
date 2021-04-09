add_library(moonlight-ndl SHARED 
    ndl_platform.c
    ndl_audio.c
    ndl_video.c
    ${CMAKE_SOURCE_DIR}/app/platform/webos/os_info.c
    ${CMAKE_SOURCE_DIR}/app/util/debugprint.c
)
target_include_directories(moonlight-ndl PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${PBNJSON_INCLUDE_DIRS}
)
target_link_libraries(moonlight-ndl moonlight-common-c opus NDL_directmedia ${PBNJSON_LIBRARIES})
target_compile_definitions(moonlight-ndl PUBLIC HAVE_NDL)

add_library(moonlight-lgnc SHARED
    lgnc_platform.c
    lgnc_audio.c
    lgnc_video.c
    ${CMAKE_SOURCE_DIR}/app/platform/webos/os_info.c
)
target_include_directories(moonlight-lgnc PRIVATE
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${PBNJSON_INCLUDE_DIRS}
)
target_link_libraries(moonlight-lgnc moonlight-common-c opus lgncopenapi AcbAPI ${PBNJSON_LIBRARIES})
target_compile_definitions(moonlight-lgnc PUBLIC HAVE_LGNC)
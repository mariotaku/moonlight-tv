cmake_minimum_required (VERSION 3.6)

add_library(moonlight-lgnc SHARED
    lgnc_platform.c
    lgnc_audio.c
    lgnc_video.c
)
target_include_directories(moonlight-lgnc PRIVATE
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${PBNJSON_INCLUDE_DIRS}
)
target_link_libraries(moonlight-lgnc PRIVATE moonlight-common-c opus lgncopenapi AcbAPI ${PBNJSON_LIBRARIES})

install(TARGETS moonlight-lgnc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
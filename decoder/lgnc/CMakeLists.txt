cmake_minimum_required (VERSION 3.6)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/assets/lib)

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
target_link_libraries(moonlight-lgnc moonlight-common-c opus lgncopenapi AcbAPI ${PBNJSON_LIBRARIES})
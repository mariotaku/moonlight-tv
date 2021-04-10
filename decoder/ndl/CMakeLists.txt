cmake_minimum_required (VERSION 3.6)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/assets/lib)

add_library(moonlight-ndl SHARED 
    ndl_platform.c
    ndl_audio.c
    ndl_video.c
)
target_include_directories(moonlight-ndl PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${PBNJSON_INCLUDE_DIRS}
)
target_link_libraries(moonlight-ndl moonlight-common-c opus NDL_directmedia ${PBNJSON_LIBRARIES})

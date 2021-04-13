cmake_minimum_required (VERSION 3.6)

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
target_link_libraries(moonlight-ndl PRIVATE moonlight-common-c opus NDL_directmedia ${PBNJSON_LIBRARIES})

install(TARGETS moonlight-ndl LIBRARY)
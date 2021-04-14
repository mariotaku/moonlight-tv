cmake_minimum_required (VERSION 3.6)

if (NOT FFMPEG_FOUND)
    #pkg_check_modules(FFMPEG REQUIRED libavcodec>=57 libavutil>=55)
endif()

add_library(moonlight-ffmpeg-sw SHARED 
    ffglrend.c
    ffmpeg.c
    ffvid.c
    ffsw_platform.c
)
target_include_directories(moonlight-ffmpeg-sw PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${PBNJSON_INCLUDE_DIRS}
)
target_link_libraries(moonlight-ffmpeg-sw PRIVATE moonlight-common-c ${FFMPEG_LIBRARIES})
target_include_directories(moonlight-ffmpeg-sw SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_include_directories(moonlight-ffmpeg-sw SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/incbin)
target_compile_definitions(moonlight-ffmpeg-sw PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

install(TARGETS moonlight-ffmpeg-sw LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
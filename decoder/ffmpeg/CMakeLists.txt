cmake_minimum_required (VERSION 3.6)

if (NOT FFMPEG_FOUND)
    pkg_check_modules(FFMPEG REQUIRED libavcodec>=57 libavutil>=55)
endif()

add_library(moonlight-ffmpeg SHARED 
    ffmpeg_platform.c
    ffmpeg_renderer.c
    ffmpeg_vdec.c
    ffmpeg.c
)
target_include_directories(moonlight-ffmpeg PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${PBNJSON_INCLUDE_DIRS}
)
target_link_libraries(moonlight-ffmpeg PRIVATE moonlight-common-c ${FFMPEG_LIBRARIES})
target_include_directories(moonlight-ffmpeg SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_include_directories(moonlight-ffmpeg SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/incbin)
target_compile_definitions(moonlight-ffmpeg PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(moonlight-ffmpeg PRIVATE -Werror=implicit-function-declaration)

install(TARGETS moonlight-ffmpeg LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
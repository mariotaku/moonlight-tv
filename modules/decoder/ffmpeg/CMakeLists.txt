cmake_minimum_required (VERSION 3.6)

if (NOT FFMPEG_FOUND)
    pkg_check_modules(FFMPEG REQUIRED libavcodec>=57 libavutil>=55)
endif()

set(FFMPEG_SRC
    ffmpeg_platform.c
    ffmpeg_renderer.c
    ffmpeg_vdec.c
    ffmpeg.c
)

if (DECODER_FFMPEG_STATIC)
    add_library(moonlight-ffmpeg STATIC ${FFMPEG_SRC})
else()
    add_library(moonlight-ffmpeg SHARED ${FFMPEG_SRC})
endif()

target_include_directories(moonlight-ffmpeg PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
)

target_link_libraries(moonlight-ffmpeg PRIVATE moonlight-common-c ${FFMPEG_LIBRARIES} ${SDL2_LIBRARIES})
target_include_directories(moonlight-ffmpeg SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS}  ${SDL2_INCLUDE_DIRS})
target_compile_definitions(moonlight-ffmpeg PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if (DECODER_FFMPEG_STATIC)
    target_compile_definitions(moonlight-ffmpeg PUBLIC DECODER_FFMPEG_STATIC=1)
endif()

if (NOT DECODER_FFMPEG_STATIC)
    install(TARGETS moonlight-ffmpeg LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
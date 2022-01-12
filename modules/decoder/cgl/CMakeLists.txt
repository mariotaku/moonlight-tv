cmake_minimum_required(VERSION 3.6)

set(DECODER_CGL_SRC
        cgl_platform.c
        cgl_audio.c
        cgl_video.c
        cgl_logging.c
        )

add_library(moonlight-cgl SHARED ${DECODER_CGL_SRC})

target_include_directories(moonlight-cgl PRIVATE
        ${CMAKE_SOURCE_DIR}/app/stream/audio
        ${CMAKE_SOURCE_DIR}/app/stream/video
        ${CMAKE_SOURCE_DIR}/app
        ${PBNJSON_INCLUDE_DIRS}
        )

target_link_libraries(moonlight-cgl PRIVATE moonlight-common-c-headers opus cgl)
target_compile_definitions(moonlight-cgl PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS moonlight-cgl LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
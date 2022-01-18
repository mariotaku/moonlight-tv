cmake_minimum_required(VERSION 3.6)

set(DECODER_CGL_SRC
        cgl_platform.c
        cgl_audio.c
        cgl_video.c
        cgl_logging.c
        )

add_library(moonlight-cgl SHARED ${DECODER_CGL_SRC})

target_link_libraries(moonlight-cgl PRIVATE moonlight-module-api opus cgl)
target_compile_definitions(moonlight-cgl PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS moonlight-cgl LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
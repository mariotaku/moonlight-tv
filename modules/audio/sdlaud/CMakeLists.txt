cmake_minimum_required(VERSION 3.6)

add_library(moonlight-sdlaud STATIC
        ringbuf.c
        sdl_audio.c
        )
target_include_directories(moonlight-sdlaud PRIVATE ${CMAKE_SOURCE_DIR}/app PRIVATE SYSTEM ${SDL2_INCLUDE_DIRS})
target_link_libraries(moonlight-sdlaud PRIVATE moonlight-common-c-headers ${SDL2_LIBRARIES})
target_compile_definitions(moonlight-sdlaud PRIVATE APPLOG_HOST=1 "$<$<CONFIG:DEBUG>:DEBUG>")

if (OPUS_LIBRARIES)
    target_include_directories(moonlight-sdlaud SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight-sdlaud PRIVATE ${OPUS_LIBRARIES})
else ()
    target_link_libraries(moonlight-sdlaud PRIVATE opus)
endif ()
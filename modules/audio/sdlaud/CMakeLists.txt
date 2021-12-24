cmake_minimum_required(VERSION 3.6)

add_library(moonlight-sdlaud STATIC
        ringbuf.c
        sdl_audio.c
        )
target_include_directories(moonlight-sdlaud PRIVATE ${CMAKE_SOURCE_DIR}/app PRIVATE SYSTEM ${SDL2_INCLUDE_DIRS})
target_link_libraries(moonlight-sdlaud PRIVATE moonlight-common-c ${SDL2_LIBRARIES})


if (OPUS_LIBRARIES)
    target_include_directories(moonlight-sdlaud SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight-sdlaud PRIVATE ${OPUS_LIBRARIES})
else ()
    target_link_libraries(moonlight-sdlaud PRIVATE opus)
endif ()

install(TARGETS moonlight-sdlaud LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
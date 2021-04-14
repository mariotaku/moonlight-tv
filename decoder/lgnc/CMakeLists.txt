cmake_minimum_required (VERSION 3.6)

set(DECODER_LGNC_SRC
    lgnc_platform.c
    lgnc_audio.c
    lgnc_video.c
)

if (DECODER_LGNC_STATIC)
    add_library(moonlight-lgnc STATIC ${DECODER_LGNC_SRC})
else()
    add_library(moonlight-lgnc SHARED ${DECODER_LGNC_SRC})
endif()

target_include_directories(moonlight-lgnc PRIVATE
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${PBNJSON_INCLUDE_DIRS}
)
target_link_libraries(moonlight-lgnc PRIVATE moonlight-common-c opus lgncopenapi)

if (DECODER_LGNC_NOINIT)
    target_compile_definitions(moonlight-lgnc PRIVATE DECODER_LGNC_NOINIT)
endif()

if (NOT DECODER_LGNC_STATIC)
    install(TARGETS moonlight-lgnc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
cmake_minimum_required (VERSION 3.6)

add_library(moonlight-ndl-webos5 SHARED
    ndl_platform.c
    ndl_audio_opus.c
    ndl_video.c
    ndl_webos5.c
    base64.c
)
target_include_directories(moonlight-ndl-webos5 PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
    ${PBNJSON_INCLUDE_DIRS}
)
target_link_libraries(moonlight-ndl-webos5 PRIVATE moonlight-common-c-headers opus NDL_directmedia)
target_compile_definitions(moonlight-ndl-webos5 PRIVATE WEBOS_TARGET_VERSION_MAJOR=5 PLUGIN_SYMBOL_SUFFIX=ndl_webos5 "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS moonlight-ndl-webos5 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
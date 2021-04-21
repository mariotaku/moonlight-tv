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
target_compile_definitions(moonlight-ndl PRIVATE PLUGIN_SYMBOL_SUFFIX=ndl)
target_compile_options(moonlight-ndl PRIVATE -Werror=implicit-function-declaration)

install(TARGETS moonlight-ndl LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_library(moonlight-ndl-webos5 SHARED 
        ndl_platform.c
        ndl_audio.c
        ndl_video.c
        ndl_webos5.c
    )
    target_include_directories(moonlight-ndl-webos5 PRIVATE 
        ${CMAKE_SOURCE_DIR}/app/stream/audio
        ${CMAKE_SOURCE_DIR}/app/stream/video
        ${CMAKE_SOURCE_DIR}/app
        ${PBNJSON_INCLUDE_DIRS}
    )
    target_link_libraries(moonlight-ndl-webos5 PRIVATE moonlight-common-c opus NDL_directmedia ${PBNJSON_LIBRARIES})
    target_compile_definitions(moonlight-ndl-webos5 PRIVATE NDL_WEBOS5 PLUGIN_SYMBOL_SUFFIX=ndl_webos5)
    target_compile_options(moonlight-ndl-webos5 PRIVATE -Werror=implicit-function-declaration)

    install(TARGETS moonlight-ndl-webos5 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
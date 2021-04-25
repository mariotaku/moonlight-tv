cmake_minimum_required (VERSION 3.6)

enable_language(CXX OPTIONAL)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)

add_library(moonlight-smp-acb SHARED
    src/DecoderPlatform.cpp
    src/AVStreamPlayer.cpp
    src/base64.cpp
)
target_include_directories(moonlight-smp-acb PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio 
    ${CMAKE_SOURCE_DIR}/app/stream/video
)
target_include_directories(moonlight-smp-acb PRIVATE ${GLIB_INCLUDE_DIRS})
target_include_directories(moonlight-smp-acb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(moonlight-smp-acb PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_link_libraries(moonlight-smp-acb PRIVATE moonlight-common-c playerAPIs Acb)
target_compile_definitions(moonlight-smp-acb PRIVATE USE_ACB PLUGIN_SYMBOL_SUFFIX=smp_acb SMP_DECODER_NS=MoonlightStarfishAcb)

install(TARGETS moonlight-smp-acb LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_library(moonlight-smp SHARED
    src/DecoderPlatform.cpp
    src/AVStreamPlayer.cpp
    src/base64.cpp
)
target_include_directories(moonlight-smp PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/platform/webos
    ${CMAKE_SOURCE_DIR}/app/stream/audio 
    ${CMAKE_SOURCE_DIR}/app/stream/video
)
target_include_directories(moonlight-smp PRIVATE ${SDL2_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
target_include_directories(moonlight-smp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(moonlight-smp PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_link_libraries(moonlight-smp PRIVATE moonlight-common-c playerAPIs ${SDL2_LIBRARIES})
target_compile_definitions(moonlight-smp PRIVATE USE_SDL_WEBOS PLUGIN_SYMBOL_SUFFIX=smp SMP_DECODER_NS=MoonlightStarfish)

install(TARGETS moonlight-smp LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
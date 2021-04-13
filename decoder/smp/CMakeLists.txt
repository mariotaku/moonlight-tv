cmake_minimum_required (VERSION 3.6)

unset(CMAKE_LIBRARY_OUTPUT_DIRECTORY)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/assets/lib)

enable_language(CXX OPTIONAL)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)

add_library(moonlight-smp-acb SHARED
    src/DecoderPlatform.cpp
    src/AbsStreamPlayer.cpp
    src/AudioStreamPlayer.cpp
    src/VideoStreamPlayer.cpp
)
target_include_directories(moonlight-smp-acb PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio 
    ${CMAKE_SOURCE_DIR}/app/stream/video
)
target_include_directories(moonlight-smp-acb PRIVATE ${GLIB_INCLUDE_DIRS})
target_include_directories(moonlight-smp-acb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(moonlight-smp-acb PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_link_libraries(moonlight-smp-acb moonlight-common-c playerAPIs Acb)
target_compile_definitions(moonlight-smp-acb PRIVATE USE_ACB DECODER_PLATFORM_NAME=smp_acb SMP_DECODER_NS=MoonlightStarfishAcb)

add_library(moonlight-smp SHARED
    src/DecoderPlatform.cpp
    src/AbsStreamPlayer.cpp
    src/AudioStreamPlayer.cpp
    src/VideoStreamPlayer.cpp
)
target_include_directories(moonlight-smp PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/platform/webos
    ${CMAKE_SOURCE_DIR}/app/stream/audio 
    ${CMAKE_SOURCE_DIR}/app/stream/video
)
target_include_directories(moonlight-smp PRIVATE ${SDL2_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
target_include_directories(moonlight-smp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(moonlight-smp PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_link_libraries(moonlight-smp moonlight-common-c playerAPIs ${SDL2_LIBRARIES})
target_compile_definitions(moonlight-smp PRIVATE USE_SDL_WEBOS DECODER_PLATFORM_NAME=smp SMP_DECODER_NS=MoonlightStarfish)
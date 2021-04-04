cmake_minimum_required (VERSION 3.6)

unset(CMAKE_LIBRARY_OUTPUT_DIRECTORY)

add_subdirectory(stub)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/assets/lib)

enable_language(CXX OPTIONAL)

add_library(moonlight-smp SHARED
    src/DecoderPlatform.cpp
    src/VideoStreamPlayer.cpp
    ${CMAKE_SOURCE_DIR}/app/platform/webos/SDL_webOS.c
)
target_include_directories(moonlight-smp PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/platform/webos
    ${CMAKE_SOURCE_DIR}/app/stream/audio 
    ${CMAKE_SOURCE_DIR}/app/stream/video
)
target_include_directories(moonlight-smp PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(moonlight-smp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(moonlight-smp moonlight-common-c playerAPIs ${SDL2_LIBRARIES})
target_compile_options(moonlight-smp PRIVATE -fPIC)
target_compile_definitions(moonlight-smp PUBLIC HAVE_SMP _GNU_SOURCE)
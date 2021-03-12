cmake_minimum_required (VERSION 3.6)

set(BUILD_SHARED_LIBS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(ndl/libndl)
add_subdirectory(lgnc/lgncapi)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/assets/lib)

target_compile_options(NDL_directmedia PRIVATE -fPIC)

add_library(moonlight-ndl SHARED ndl/ndl_platform.c ndl/ndl_audio.c ndl/ndl_video.c)
target_include_directories(moonlight-ndl PRIVATE ${CMAKE_SOURCE_DIR}/app/stream/audio ${CMAKE_SOURCE_DIR}/app/stream/video)
target_link_libraries(moonlight-ndl moonlight-common-c opus NDL_directmedia)
target_compile_options(moonlight-ndl PRIVATE -fPIC)
target_compile_definitions(moonlight-ndl PUBLIC HAVE_NDL)

target_compile_options(NDL_directmedia PRIVATE -fPIC)

add_library(moonlight-ndl SHARED ndl/ndl_platform.c ndl/ndl_audio.c ndl/ndl_video.c)
target_include_directories(moonlight-ndl PRIVATE ${CMAKE_SOURCE_DIR}/app/stream/audio ${CMAKE_SOURCE_DIR}/app/stream/video)
target_link_libraries(moonlight-ndl moonlight-common-c opus NDL_directmedia)
target_compile_options(moonlight-ndl PRIVATE -fPIC)
target_compile_definitions(moonlight-ndl PUBLIC HAVE_NDL)
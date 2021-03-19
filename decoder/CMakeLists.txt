cmake_minimum_required (VERSION 3.6)

if (TARGET_WEBOS)
    add_subdirectory(ndl/stub)
    add_subdirectory(lgnc/stub)
    add_subdirectory(smp/stub)

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/assets/lib)

    target_compile_options(NDL_directmedia PRIVATE -fPIC)
    target_compile_options(lgncopenapi PRIVATE -fPIC)

    add_library(moonlight-ndl SHARED ndl/ndl_platform.c ndl/ndl_audio.c ndl/ndl_video.c)
    target_include_directories(moonlight-ndl PRIVATE ${CMAKE_SOURCE_DIR}/app/stream/audio ${CMAKE_SOURCE_DIR}/app/stream/video)
    target_link_libraries(moonlight-ndl moonlight-common-c opus NDL_directmedia)
    target_compile_options(moonlight-ndl PRIVATE -fPIC)
    target_compile_definitions(moonlight-ndl PUBLIC HAVE_NDL)


    add_library(moonlight-lgnc SHARED lgnc/lgnc_platform.c lgnc/lgnc_audio.c lgnc/lgnc_video.c)
    target_include_directories(moonlight-lgnc PRIVATE ${CMAKE_SOURCE_DIR}/app/stream/audio ${CMAKE_SOURCE_DIR}/app/stream/video)
    target_link_libraries(moonlight-lgnc moonlight-common-c opus lgncopenapi)
    target_compile_options(moonlight-lgnc PRIVATE -fPIC)
    target_compile_definitions(moonlight-lgnc PUBLIC HAVE_LGNC)

    project(moonlight-decoder-smp CXX)

    add_library(moonlight-smp SHARED 
        smp/smp_platform.c
        smp/smp_player.c
        smp/smp_audio.c
        smp/smp_video.c
        smp/stub/sample/src/StarfishDirectMediaPlayer.cpp
        ${CMAKE_SOURCE_DIR}/app/platform/webos/SDL_webOS.c
    )
    target_include_directories(moonlight-smp PRIVATE 
        ${CMAKE_SOURCE_DIR}/app/platform/webos
        ${CMAKE_SOURCE_DIR}/app/stream/audio 
        ${CMAKE_SOURCE_DIR}/app/stream/video
    )
    target_include_directories(moonlight-smp PRIVATE ${SDL2_INCLUDE_DIRS})
    target_include_directories(moonlight-smp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/smp/stub/sample/src)
    target_link_libraries(moonlight-smp moonlight-common-c playerAPIs ${SDL2_LIBRARIES})
    target_compile_options(moonlight-smp PRIVATE -fPIC)
    target_compile_definitions(moonlight-smp PUBLIC HAVE_SMP _GNU_SOURCE)

endif(TARGET_WEBOS)
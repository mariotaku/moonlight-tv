cmake_minimum_required (VERSION 3.6)
project(moonlight 
    VERSION 0.6.2
    LANGUAGES C
)

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH 
    ${CMAKE_SOURCE_DIR}/utils/cmake
    ${CMAKE_SOURCE_DIR}/cmake
)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(CC_BASENAME ${CMAKE_C_COMPILER} NAME)

option(TARGET_WEBOS "Build for LG webOS" OFF)
option(TARGET_RASPI "Build for Raspberry Pi" OFF)

option(BUILD_CUSTOM_SDL "Include custom SDL" OFF)

set(OPUS_BUILD_SHARED_LIBRARY ON)

if (CC_BASENAME MATCHES "^arm-webos-linux")
    set(TARGET_WEBOS ON)
    set(CMAKE_C_COMPILER_ID "GNU")
endif()

include(LintOptions)

if (TARGET_WEBOS)
    if (NOT ENV{ARCH})
        string(REPLACE "-" ";" CC_BASENAME_SEGS ${CC_BASENAME})
        list(GET CC_BASENAME_SEGS 0 CC_ARCH)
        if (CC_ARCH MATCHES [[i.86]])
            set(ENV{ARCH} "x86")
            set(ENV{PKG_CONFIG_PATH} "/usr/lib/i386-linux-gnu/pkgconfig")
        else()
            set(ENV{ARCH} ${CC_ARCH})
        endif()
    endif()
    set(THREADS_PTHREAD_ARG "2" CACHE STRING "Result from TRY_RUN" FORCE)
    set(HAVE_SDL TRUE)
    
    add_subdirectory(system/webos/tv-native-apis)
elseif (TARGET_RASPI)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/vc/lib/pkgconfig/")
    set(TARGET_LINUX TRUE)
    set(HAVE_SDL TRUE)
    option(GS_SPS "" ON)
    add_subdirectory(third_party/h264bitstream)
    add_subdirectory(third_party/ilclient)
else()
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
        set(TARGET_DARWIN TRUE)
    else()
        set(TARGET_LINUX TRUE)
    endif()
    
    set(TARGET_DESKTOP TRUE)
    set(HAVE_SDL TRUE)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(UUID REQUIRED uuid)
pkg_check_modules(EXPAT REQUIRED expat)


if (TARGET_WEBOS)
    pkg_check_modules(GLES REQUIRED egl glesv2)
    add_subdirectory(third_party/opus)
    target_compile_options(opus PRIVATE -fPIC)
    set_target_properties(opus PROPERTIES 
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/assets/lib
    )
    add_subdirectory(prebuilt/webos)

    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(PBNJSON REQUIRED pbnjson_c)
    pkg_check_modules(LUNA_SERVICE REQUIRED luna-service2)
elseif (TARGET_RASPI)
    find_package(Broadcom REQUIRED)

    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(GLES REQUIRED bcm_host brcmglesv2)
    pkg_check_modules(FFMPEG REQUIRED libavcodec>=57 libavutil>=55)
    pkg_check_modules(OPUS REQUIRED opus)
elseif (TARGET_DESKTOP)
    set(OpenGL_GL_PREFERENCE "GLVND")
    find_package(OpenGL REQUIRED)
    
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(FFMPEG REQUIRED libavcodec>=57 libavutil>=55)
    pkg_check_modules(OPUS REQUIRED opus)
endif()

set(ENABLE_PROGRAMS OFF)
set(ENABLE_TESTING OFF)
add_subdirectory(third_party/mbedtls)

add_subdirectory(core/moonlight-common-c)
add_subdirectory(core/libgamestream)
add_subdirectory(decoder)

set(SOURCES
    app/main.c
    app/backend/backend_root.c
    app/backend/computer_manager.c
    app/backend/pcmanager/pairing.c
    app/backend/pcmanager/discovery.c
    app/backend/pcmanager/wol.c
    app/backend/application_manager.c
    app/backend/coverloader.c
    app/backend/error_manager.c
    app/backend/types_impl.c
    app/stream/platform.c
    app/stream/session.c
    app/stream/connection.c
    app/stream/settings.c
    app/ui/root.c
    app/ui/fonts.c
    app/ui/launcher/window.c
    app/ui/launcher/pclist.c
    app/ui/launcher/pcempty.c
    app/ui/launcher/applist.c
    app/ui/launcher/statbar.c
    app/ui/launcher/modals.c
    app/ui/launcher/pairing.c
    app/ui/streaming/overlay.c
    app/ui/streaming/bottombar.c
    app/stream/video/dummy.c
    app/ui/settings/window.c
    app/ui/settings/pane_basic.c
    app/ui/settings/pane_host.c
    app/ui/settings/pane_mouse.c
    app/ui/settings/pane_about.c
    app/ui/settings/statbar.c
    app/platform/webos/sprites_webos.c
    app/util/lruc.c
    app/util/path.c
    app/util/navkey.c
    app/util/debugprint.c
)

if (SDL2_IMAGE_FOUND)
    list(APPEND SOURCES
        app/nuklear/nkimage_sdl.c
    )
else()
    list(APPEND SOURCES
        app/nuklear/nkimage_stb.c
    )
endif()

if (HAVE_SDL)
    list(APPEND SOURCES
        app/app_sdl.c
        app/backend/pcmanager/sdl_impl.c
        app/stream/input/sdlinput.c
        app/stream/input/sdl/keyboard.c
        app/stream/input/sdl/gamepad.c
        app/stream/input/sdl/mouse.c
        app/platform/sdl/bus.c
        app/platform/sdl/input_manager.c
        app/platform/sdl/navkey_sdl.c
        app/stream/audio/sdl.c
        app/stream/video/delegate.c
        app/stream/sdl_platform.c
    )
    if (TARGET_WEBOS)
        list(APPEND SOURCES
            app/platform/webos/app_init.c
            app/stream/input/sdl/keyboard_webos.c
            app/platform/webos/navkey_sdl_webos.c
            app/platform/webos/path_webos.c
            app/platform/webos/os_info.c
            app/platform/webos/SDL_webOS_compat.c
        )
    else()
        list(APPEND SOURCES
            app/platform/sdl/path_sdl.c
        )
    endif()

    if (FFMPEG_FOUND)
        list(APPEND SOURCES
            app/ui/sdl_renderer.c
            app/stream/video/sdl.c
            app/stream/video/ffmpeg.c
        )
    endif()
endif()

add_executable(moonlight ${SOURCES})

target_compile_lint(moonlight)

set_target_properties(moonlight PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED TRUE
)

target_compile_definitions(moonlight PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
string(LENGTH ${CMAKE_SOURCE_DIR} SOURCE_DIR_LENGTH)
target_compile_definitions(moonlight PRIVATE SOURCE_DIR_LENGTH=${SOURCE_DIR_LENGTH})
target_compile_definitions(moonlight PRIVATE APP_VERSION="${PROJECT_VERSION}")

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/core/libgamestream)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/Nuklear)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/mdns)
target_include_directories(moonlight SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/message_queue)
target_include_directories(moonlight SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/incbin)

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/utils/include)

target_link_libraries(moonlight PRIVATE
 ${CURL_LIBRARIES} ${UUID_LIBRARIES} ${EXPAT_LIBRARIES}
 m Threads::Threads
 moonlight-common-c gamestream
)

target_link_libraries(moonlight PRIVATE mbedtls)

if (TARGET_WEBOS)
    target_compile_definitions(moonlight PUBLIC OS_WEBOS _GNU_SOURCE)
    target_compile_definitions(moonlight PUBLIC NK_SDL_GLES2)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL HAVE_FFMPEG)

    target_link_libraries(moonlight PRIVATE ${GLES_LIBRARIES})
    target_link_libraries(moonlight PRIVATE moonlight-smp)
    target_link_libraries(moonlight PRIVATE moonlight-smp-acb)
    target_link_libraries(moonlight PRIVATE moonlight-ndl)
    target_link_libraries(moonlight PRIVATE moonlight-lgnc)
    target_link_libraries(moonlight PRIVATE debug moonlight-dile)
    target_link_libraries(moonlight PRIVATE debug moonlight-dile-legacy)
    
    target_link_libraries(moonlight PRIVATE libavcodec libavutil opus)

    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
    target_include_directories(moonlight SYSTEM PRIVATE ${PBNJSON_INCLUDE_DIRS} ${LUNA_SERVICE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${PBNJSON_LIBRARIES} ${LUNA_SERVICE_LIBRARIES})
elseif(TARGET_RASPI)
    target_compile_definitions(moonlight PUBLIC TARGET_RASPI _GNU_SOURCE)
    target_compile_definitions(moonlight PUBLIC NK_SDL_GLES2)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL SDL_VIDEO_DRIVER_RASPI)
    target_compile_definitions(moonlight PUBLIC HAVE_PI HAVE_MMAL)

    target_compile_options(moonlight PRIVATE ${GLES_CFLAGS})
    target_include_directories(moonlight SYSTEM PRIVATE ${GLES_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${GLES_LIBRARIES})
    target_link_options(moonlight PRIVATE ${GLES_LDFLAGS})

    target_link_libraries(moonlight PRIVATE moonlight-pi)

    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

    target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})

    target_include_directories(moonlight SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${FFMPEG_LIBRARIES})

else()
    target_compile_definitions(moonlight PUBLIC TARGET_DESKTOP)
    target_compile_definitions(moonlight PUBLIC NK_SDL_GL2)
    
    if(TARGET_DARWIN)
        target_compile_definitions(moonlight PUBLIC OS_DARWIN GL_SILENCE_DEPRECATION)
    elseif(TARGET_LINUX)
        target_compile_definitions(moonlight PUBLIC OS_LINUX _GNU_SOURCE)
    endif()

    target_compile_definitions(moonlight PUBLIC HAVE_SDL HAVE_FFMPEG)

    target_link_libraries(moonlight PRIVATE mbedtls)
    target_link_libraries(moonlight PRIVATE OpenGL::GL)

    target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})

    target_include_directories(moonlight SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${FFMPEG_LIBRARIES})
    
    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
endif()

add_custom_target(moonlight-spritegen
    COMMAND nk-spritegen -n ui -i res/sprites -o res/ -c app/nuklear/ -s 1 -s 2 -s 3
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (TARGET_WEBOS)
    include(WebOSPackage)

    add_custom_target(webos-generate-gamecontrollerdb
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/webos/gen_gamecontrollerdb.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    set_target_properties(moonlight PROPERTIES
        WEBOS_APPINFO_ID com.limelight.webos
        WEBOS_APPINFO_VERSION ${PROJECT_VERSION}
        WEBOS_APPINFO_TITLE "Moonlight"
        WEBOS_APPINFO_VENDOR "Mariotaku Lee"
        WEBOS_APPINFO_ICON ${CMAKE_CURRENT_SOURCE_DIR}/deploy/webos/icon.png
        WEBOS_PACKAGE_ASSETS "assets;${CMAKE_BINARY_DIR}/assets/lib"
    )

    target_webos_package(moonlight)

    if ($ENV{CI})
        get_target_property(package_filename moonlight WEBOS_PACKAGE_FILENAME)
        get_target_property(package_target moonlight WEBOS_PACKAGE_TARGET)
        add_custom_command(TARGET ${package_target} POST_BUILD
            COMMAND echo RELEASE_FILENAME=${package_filename} >> ${CMAKE_CURRENT_BINARY_DIR}/github-release.env
        )
    else()
        include(WebOSInstall)
        target_webos_install(moonlight)
    endif()

    # add_dependencies(webos-package-moonlight webos-generate-gamecontrollerdb)
endif()
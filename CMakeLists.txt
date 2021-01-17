cmake_minimum_required (VERSION 3.6)
project(moonlight 
    VERSION 0.1.2
    LANGUAGES C
)

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/utils/cmake)

get_filename_component(CC_BASENAME ${CMAKE_C_COMPILER} NAME)

if (CC_BASENAME MATCHES "^arm-webos-")
    option(LGNC_LEGACY "Build for LG NetCast Platform" OFF)
    if (LGNC_LEGACY)
        set(TARGET_NETCAST TRUE)
    else()
        set(TARGET_WEBOS TRUE)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
    set(TARGET_DARWIN TRUE)
    set(TARGET_DESKTOP TRUE)
    set(ENV{OPENSSL_ROOT_DIR} "/usr/local/opt/openssl@1.1")
else()
    set(TARGET_LINUX TRUE)
    set(TARGET_DESKTOP TRUE)
endif()

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2 SDL2_image)
pkg_check_modules(cURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(UUID REQUIRED IMPORTED_TARGET uuid)
pkg_check_modules(Expat REQUIRED IMPORTED_TARGET expat)

if (TARGET_WEBOS)
    pkg_check_modules(GLES REQUIRED IMPORTED_TARGET egl glesv2)
    pkg_check_modules(OpenSSL REQUIRED IMPORTED_TARGET openssl)
    add_subdirectory(third_party/opus)
    add_subdirectory(system/libndl)
    add_subdirectory(system/lgncapi)
elseif (TARGET_DESKTOP)
    set(OpenGL_GL_PREFERENCE "GLVND")
    find_package(OpenSSL REQUIRED)
    find_package(OpenGL REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec>=57 libavutil>=55)
    pkg_check_modules(Opus opus REQUIRED IMPORTED_TARGET opus)
endif()

add_subdirectory(core/moonlight-common-c)

set(SOURCES
    app/main.c
    app/backend/backend_root.c
    app/backend/computer_manager.c
    app/backend/computer_discovery.c
    app/backend/application_manager.c
    app/backend/coverloader.c
    app/backend/error_manager.c
    app/stream/platform.c
    app/stream/session.c
    app/stream/connection.c
    app/stream/settings.c
    app/ui/gui_root.c
    app/ui/launcher_window.c
    app/ui/streaming_overlay.c
    app/ui/settings_window.c
    app/ui/widget/applist.c
    app/util/lruc.c
    app/util/path.c
    app/util/gs/clientex.c
    core/moonlight-embedded/libgamestream/http.c
    core/moonlight-embedded/libgamestream/mkcert.c
    core/moonlight-embedded/libgamestream/xml.c
)

if (TARGET_WEBOS)
    list(APPEND SOURCES
        app/app_sdl.c
        app/platform/webos/app_init.c
        app/platform/sdl/webos_keys.c
        app/stream/input/sdlinput.c
        app/platform/sdl/bus.c
        app/platform/sdl/nkimage.c
        app/platform/sdl/input_manager.c
        app/platform/webos/navkey_sdl_webos.c
        app/stream/audio/ndl.c
        app/stream/video/ndl.c
        app/stream/audio/lgnc.c
        app/stream/video/lgnc.c
    )
elseif (TARGET_DESKTOP)
    list(APPEND SOURCES
        app/app_sdl.c
        app/stream/input/sdlinput.c
        app/platform/sdl/bus.c
        app/platform/sdl/nkimage.c
        app/platform/sdl/input_manager.c
        app/platform/sdl/navkey_sdl.c
        core/moonlight-embedded/src/sdl.c
        core/moonlight-embedded/src/audio/sdl.c
        core/moonlight-embedded/src/video/ffmpeg.c
        core/moonlight-embedded/src/video/sdl.c
    )
endif()

add_executable(moonlight ${SOURCES})

set_target_properties(moonlight PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED TRUE
)

target_compile_definitions(moonlight PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/core/moonlight-embedded)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/Nuklear)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/mdns)
target_include_directories(moonlight SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/message_queue)
target_include_directories(moonlight SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/incbin)

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/utils/include)

target_link_libraries(moonlight PRIVATE
 PkgConfig::cURL PkgConfig::UUID PkgConfig::Expat
 m pthread
 moonlight-common-c
)

if (TARGET_WEBOS)
    target_compile_definitions(moonlight PUBLIC OS_WEBOS)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL)
    target_link_libraries(moonlight PRIVATE PkgConfig::SDL2 PkgConfig::OpenSSL PkgConfig::GLES opus)
    target_link_libraries(moonlight PRIVATE NDL_directmedia lgncopenapi)
elseif(TARGET_DARWIN)
    target_compile_definitions(moonlight PUBLIC OS_DARWIN)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL)

    get_filename_component(SDL2_INCLUDE_DIR_PARENT ${SDL2_INCLUDE_DIRS} DIRECTORY)
    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIR_PARENT})
    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})

    target_link_libraries(moonlight PRIVATE PkgConfig::SDL2 PkgConfig::Opus PkgConfig::FFMPEG OpenGL::GL OpenSSL::Crypto)
else()
    target_compile_definitions(moonlight PUBLIC OS_LINUX _GNU_SOURCE)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL)

    target_link_libraries(moonlight PRIVATE PkgConfig::SDL2 PkgConfig::Opus PkgConfig::FFMPEG OpenGL::GL OpenSSL::Crypto)
endif()


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
        WEBOS_PACKAGE_ASSETS "assets;legacy-prebuilt/lib" 
    )

    target_webos_package(moonlight)

    add_custom_command(TARGET webos-package-moonlight POST_BUILD
        COMMAND echo RELEASE_FILENAME=${moonlight_WEBOS_PACKAGE_FILENAME} >> ${CMAKE_CURRENT_BINARY_DIR}/github-release.env
    )

    # add_dependencies(webos-package-moonlight webos-generate-gamecontrollerdb)
endif()
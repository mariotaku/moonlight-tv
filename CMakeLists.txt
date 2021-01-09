cmake_minimum_required (VERSION 3.0)

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/utils/cmake)

get_filename_component(CC_BASENAME ${CMAKE_C_COMPILER} NAME)

if (CC_BASENAME MATCHES "^arm-webos-")
    set(TARGET_WEBOS TRUE)
    include(WebOSPackage)
else() 
    set(TARGET_LINUX TRUE)
endif()

option(WEBOS_LEGACY "Use Legacy API" OFF)

project(moonlight LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(OpenSSL REQUIRED IMPORTED_TARGET openssl)
pkg_check_modules(cURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(UUID REQUIRED IMPORTED_TARGET uuid)
pkg_check_modules(Expat REQUIRED IMPORTED_TARGET expat)

if (TARGET_WEBOS)
    pkg_check_modules(GLES REQUIRED IMPORTED_TARGET egl glesv2)
    add_subdirectory(third_party/opus)
    if (NOT WEBOS_LEGACY)
        add_subdirectory(system/libndl)
    else()
        add_subdirectory(system/lgncapi)
    endif()
elseif (TARGET_LINUX)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec>=58 libavutil>=56)
    find_package(OpenGL REQUIRED)
    pkg_check_modules(Opus opus REQUIRED IMPORTED_TARGET opus)
endif()

add_subdirectory(core/moonlight-common-c)

set(SOURCES
    app/main.c
    app/backend/backend_root.c
    app/backend/computer_manager.c
    app/backend/computer_discovery.c
    app/backend/application_manager.c
    app/backend/error_manager.c
    app/stream/platform.c
    app/stream/session.c
    app/stream/connection.c
    app/stream/settings.c
    app/ui/gui_root.c
    app/ui/launcher_window.c
    app/ui/applications_list.c
    app/ui/streaming_overlay.c
    app/ui/settings_window.c
    core/moonlight-embedded/src/config.c
    core/moonlight-embedded/libgamestream/client.c
    core/moonlight-embedded/libgamestream/http.c
    core/moonlight-embedded/libgamestream/mkcert.c
    core/moonlight-embedded/libgamestream/xml.c
)

if (TARGET_WEBOS)
    list(APPEND SOURCES
        app/sdl/webos_keys.c
    )
    if (NOT WEBOS_LEGACY)
        list(APPEND SOURCES
            app/app_sdl.c
            app/stream/audio/ndl.c
            app/stream/video/ndl.c
            app/stream/input/sdl.c
            app/util/bus_sdl.c
        )
    else()
        list(APPEND SOURCES
            app/stream/audio/lgnc.c
            app/stream/video/lgnc.c
        )
    endif()
elseif (TARGET_LINUX)
    list(APPEND SOURCES
        app/app_sdl.c
        app/stream/input/sdl.c
        app/util/bus_sdl.c
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
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/core/moonlight-embedded/src)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/Nuklear)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/mdns)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/utils/include)

target_link_libraries(moonlight PRIVATE
 PkgConfig::SDL2 PkgConfig::OpenSSL PkgConfig::cURL PkgConfig::UUID PkgConfig::Expat
 m pthread
 moonlight-common-c
)

if (TARGET_WEBOS)
    target_compile_definitions(moonlight PUBLIC OS_WEBOS)
    target_link_libraries(moonlight PRIVATE PkgConfig::GLES opus)
    if (NOT WEBOS_LEGACY)
        target_compile_definitions(moonlight PUBLIC HAVE_SDL)
        target_compile_definitions(moonlight PUBLIC USE_NDL)
        target_link_libraries(moonlight PRIVATE NDL_directmedia)
    else()
        target_compile_definitions(moonlight PUBLIC WEBOS_LEGACY USE_LGNCAPI)
        target_link_libraries(moonlight PRIVATE lgncopenapi)
    endif()
else()
    target_compile_definitions(moonlight PUBLIC OS_LINUX)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL)
    target_link_libraries(moonlight PRIVATE PkgConfig::Opus PkgConfig::FFMPEG OpenGL)
endif()


if (TARGET_WEBOS)

    add_custom_target(webos-generate-gamecontrollerdb
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/webos/gen_gamecontrollerdb.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    set_target_properties(moonlight PROPERTIES
        WEBOS_APPINFO_ID com.limelight.webos
        WEBOS_APPINFO_VERSION 0.0.3
        WEBOS_APPINFO_VENDOR "Mariotaku Lee"
        WEBOS_APPINFO_ICON ${CMAKE_CURRENT_SOURCE_DIR}/deploy/webos/icon.png
        WEBOS_APPINFO_TITLE "Moonlight"
        WEBOS_APPINFO_EXTRA "nativeLifeCycleInterfaceVersion=2"
        WEBOS_PACKAGE_ASSETS "assets"
    )

    target_webos_package(moonlight)

    # add_dependencies(webos-package-moonlight webos-generate-gamecontrollerdb)
endif()
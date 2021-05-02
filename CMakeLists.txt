cmake_minimum_required (VERSION 3.6)
project(moonlight 
    VERSION 0.7.9
    LANGUAGES C
)

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH 
    ${CMAKE_SOURCE_DIR}/utils/cmake
    ${CMAKE_SOURCE_DIR}/cmake
)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(CC_BASENAME ${CMAKE_C_COMPILER} NAME)

option(TARGET_DESKTOP "Build for Desktop" OFF)
option(TARGET_WEBOS "Build for LG webOS" OFF)
option(TARGET_LGNC "Build for LG NetCast" OFF)
option(TARGET_RASPI "Build for Raspberry Pi" OFF)

option(OPUS_INSTALL_CMAKE_CONFIG_MODULE "" OFF)
option(OPUS_INSTALL_PKG_CONFIG_MODULE "" OFF)

option(FDK_AAC_INSTALL_CMAKE_CONFIG_MODULE "" OFF)
option(FDK_AAC_INSTALL_PKGCONFIG_MODULE "" OFF)

option(USE_MBEDTLS "" ON)
option(USE_WERROR "" OFF)

option(ENABLE_PROGRAMS "Skip mbedtls programs" OFF)
option(ENABLE_TESTING "Skip mbedtls testing" OFF)
option(INSTALL_MBEDTLS_HEADERS "Don't install mbedtls headers" OFF)
option(MBEDTLS_FATAL_WARNINGS "Turn mbedtls warning as errors off" OFF)

option(MDNS_BUILD_EXAMPLE "" OFF)

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

    set(WEBOS_APPINFO_ID "com.limelight.webos")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/webos-unused-prefix)
    set(CMAKE_INSTALL_WEBOS_PKGDIR ${CMAKE_BINARY_DIR}/webos_pkg_$ENV{ARCH})
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_WEBOS_PKGDIR}/lib)
    set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_WEBOS_PKGDIR})
    set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_WEBOS_PKGDIR})
elseif(TARGET_LGNC)
    set(HAVE_SDL FALSE)
    set(HAVE_LGNC TRUE)

    set(WEBOS_APPINFO_ID "com.limelight.netcast")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/lgnc-unused-prefix)
    set(CMAKE_INSTALL_WEBOS_PKGDIR ${CMAKE_BINARY_DIR}/lgnc_pkg_$ENV{ARCH})
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_WEBOS_PKGDIR}/lib)
    set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_WEBOS_PKGDIR})
    set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_WEBOS_PKGDIR})
elseif (TARGET_RASPI)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/vc/lib/pkgconfig/")
    set(TARGET_LINUX TRUE)
    set(HAVE_SDL TRUE)
elseif (TARGET_DESKTOP)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") 
        set(TARGET_DARWIN TRUE)
    else()
        set(TARGET_LINUX TRUE)
    endif()

    set(HAVE_SDL TRUE)
    
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
    set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_PREFIX}/share)
else()
    message(FATAL_ERROR "Unknown build platform")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(UUID REQUIRED uuid)
pkg_check_modules(EXPAT REQUIRED expat)

if (TARGET_WEBOS)
    set(BUILD_SHARED_LIBS ON)
    add_subdirectory(third_party/opus)
    add_subdirectory(third_party/fdk-aac)
    unset(BUILD_SHARED_LIBS)

    add_subdirectory(third_party/mbedtls)
    add_subdirectory(system/webos/tv-native-apis)

    pkg_check_modules(GLES REQUIRED egl glesv2)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(PBNJSON REQUIRED pbnjson_c)
    pkg_check_modules(LUNA_SERVICE REQUIRED luna-service2)
elseif (TARGET_LGNC)
    set(BUILD_SHARED_LIBS ON)
    add_subdirectory(third_party/opus)
    add_subdirectory(third_party/fdk-aac)
    unset(BUILD_SHARED_LIBS)

    set(DECODER_LGNC_NOINIT TRUE)
    set(DECODER_LGNC_STATIC TRUE)
    add_subdirectory(third_party/mbedtls)
    add_subdirectory(system/webos/tv-native-apis)

    pkg_check_modules(GLES REQUIRED egl glesv2)
elseif (TARGET_RASPI)
    add_subdirectory(third_party/h264bitstream)
    add_subdirectory(third_party/ilclient)

    find_package(Broadcom REQUIRED)
    find_package(MbedTLS REQUIRED)

    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(GLES REQUIRED glesv2)
    pkg_check_modules(OPUS REQUIRED opus)
elseif (TARGET_DESKTOP)
    set(OpenGL_GL_PREFERENCE "GLVND")
    find_package(OpenGL REQUIRED)
    find_package(MbedTLS REQUIRED)
    
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(OPUS REQUIRED opus)
else()
    message(FATAL_ERROR "Unknown build platform")
endif()

set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(core/moonlight-common-c)
    add_subdirectory(core/libgamestream)
unset(BUILD_SHARED_LIBS)

add_subdirectory(decoder)
add_subdirectory(audio)

if (DEFINED CMAKE_INSTALL_DATAROOTDIR)
    set(CMAKE_INSTALL_DATAROOTDIR_BACKUP ${CMAKE_INSTALL_DATAROOTDIR})
    set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_BINARY_DIR}/mdns-unused)
endif()

# add_subdirectory(third_party/mdns)

if (CMAKE_INSTALL_DATAROOTDIR_BACKUP)
    set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_DATAROOTDIR_BACKUP})
    unset(CMAKE_INSTALL_DATAROOTDIR_BACKUP)
endif()

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
    app/stream/module/module.c
    app/stream/module/audio.c
    app/stream/module/decoder.c
    app/stream/session.c
    app/stream/connection.c
    app/stream/settings.c
    app/stream/video/delegate.c
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
    app/ui/settings/pane_adv_codec.c
    app/ui/settings/pane_about.c
    app/ui/settings/statbar.c
    app/platform/webos/sprites_webos.c
    app/util/lruc.c
    app/util/path.c
    app/util/navkey.c
    app/util/debugprint.c
    app/util/logging.c
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
        app/stream/audio/sdlaud.c
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
elseif (HAVE_LGNC)
    list(APPEND SOURCES
        app/app_lgnc.c
        app/backend/pcmanager/lgnc_impl.c
        app/stream/input/lgncinput.c
        app/stream/lgnc_platform.c
        app/platform/lgnc/bus.c
        app/platform/lgnc/callbacks.c
        app/platform/lgnc/graphics.c
        app/platform/lgnc/input_manager.c
        app/platform/lgnc/path_lgnc.c
        third_party/message_queue/message_queue.c
    )
else()
    message(FATAL_ERROR "No suitable application library found - SDL2 or LGNC required")
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
target_compile_definitions(moonlight PRIVATE APP_VERSION="${PROJECT_VERSION}" APPLOG_HOST)

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/core/libgamestream)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/Nuklear)
target_include_directories(moonlight SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/message_queue)
target_include_directories(moonlight SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/incbin)
target_include_directories(moonlight SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/mdns)

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/utils/include)

target_link_libraries(moonlight PRIVATE
 ${CURL_LIBRARIES} ${UUID_LIBRARIES} ${EXPAT_LIBRARIES} 
 m ${CMAKE_DL_LIBS} Threads::Threads
 moonlight-common-c gamestream  # mdns
)

function(moonlight_add_module)
    foreach(module ${ARGN})
        if (NOT TARGET ${module})
            continue()
        endif()
        target_compile_lint(${module})
        add_dependencies(moonlight ${module})
    endforeach()
endfunction()

if (TARGET_WEBOS)
    target_compile_definitions(moonlight PUBLIC TARGET_WEBOS _GNU_SOURCE
        APPID="${WEBOS_APPINFO_ID}" 
        APPLOG_FILE=app_logfile
    )
    target_compile_definitions(moonlight PUBLIC NK_SDL_GLES2 HAVE_GLES2)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL)

    target_link_libraries(moonlight PRIVATE ${GLES_LIBRARIES})
    moonlight_add_module(moonlight-smp moonlight-smp-webos4 moonlight-smp-webos3)
    moonlight_add_module(moonlight-dile-legacy)
    moonlight_add_module(moonlight-ndl-webos5 moonlight-ndl)
    moonlight_add_module(moonlight-lgnc)
    moonlight_add_module(moonlight-pulse)
    moonlight_add_module(moonlight-ndlaud)
    moonlight_add_module(moonlight-ndlaud-webos5)
    
    target_link_libraries(moonlight PRIVATE opus)
    target_link_libraries(moonlight PRIVATE mbedtls)

    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

    target_include_directories(moonlight SYSTEM PRIVATE ${PBNJSON_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${PBNJSON_LIBRARIES})
elseif (TARGET_LGNC)
    target_compile_definitions(moonlight PUBLIC TARGET_LGNC _GNU_SOURCE 
        APPID="${WEBOS_APPINFO_ID}"
        APPLOG_FILE=app_logfile
    )
    target_compile_definitions(moonlight PUBLIC NK_LGNC_GLES2 HAVE_GLES2)

    target_link_libraries(moonlight PRIVATE mbedtls)

    target_link_libraries(moonlight PRIVATE ${GLES_LIBRARIES})
    target_link_libraries(moonlight PRIVATE lgncopenapi)
    target_link_libraries(moonlight PRIVATE moonlight-lgnc)
    
    target_link_libraries(moonlight PRIVATE opus)
elseif(TARGET_RASPI)
    target_compile_definitions(moonlight PUBLIC TARGET_RASPI)
    target_compile_definitions(moonlight PUBLIC NK_SDL_GLES2 HAVE_GLES2 _GNU_SOURCE)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL)
    target_compile_definitions(moonlight PUBLIC HAVE_PI HAVE_MMAL)

    target_compile_options(moonlight PRIVATE ${GLES_CFLAGS})
    target_include_directories(moonlight SYSTEM PRIVATE ${GLES_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${MBEDCRYPTO_LIBRARY})
    target_include_directories(moonlight SYSTEM PRIVATE ${MBEDTLS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${GLES_LIBRARIES})
    target_link_options(moonlight PRIVATE ${GLES_LDFLAGS})

    target_link_libraries(moonlight PRIVATE moonlight-pi)
    target_link_libraries(moonlight PRIVATE moonlight-ffmpeg)
    target_compile_definitions(moonlight-ffmpeg PRIVATE HAVE_GLES2)

    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

    target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})

    target_compile_options(moonlight PRIVATE "-pg")
    target_link_options(moonlight PRIVATE "-pg")

elseif (TARGET_DESKTOP)
    target_compile_definitions(moonlight PUBLIC TARGET_DESKTOP)
    target_compile_definitions(moonlight PUBLIC NK_SDL_GL2 HAVE_GL2)
    
    if(TARGET_DARWIN)
        target_compile_definitions(moonlight PUBLIC OS_DARWIN GL_SILENCE_DEPRECATION)
    elseif(TARGET_LINUX)
        target_compile_definitions(moonlight PUBLIC OS_LINUX _GNU_SOURCE)
    endif()

    target_compile_definitions(moonlight PUBLIC HAVE_SDL)

    target_link_libraries(moonlight PRIVATE mbedtls)
    target_link_libraries(moonlight PRIVATE OpenGL::GL)
    
    target_link_libraries(moonlight PRIVATE moonlight-ffmpeg)
    target_compile_definitions(moonlight-ffmpeg PRIVATE HAVE_GL2)

    target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})
    
    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
else()
    message(FATAL_ERROR "Unknown build platform")
endif()

install(TARGETS moonlight DESTINATION ${CMAKE_INSTALL_BINDIR})

add_custom_target(moonlight-spritegen
    COMMAND nk-spritegen -n ui -i res/sprites -o res/ -c app/nuklear/ -s 1 -s 2 -s 3
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (TARGET_WEBOS OR TARGET_LGNC)
    # Copy asset dir to package root
    install(DIRECTORY assets DESTINATION ${CMAKE_INSTALL_DATAROOTDIR} PATTERN ".*" EXCLUDE)
    # Copy all files under deploy/webos/ to package root
    install(DIRECTORY deploy/webos/ DESTINATION ${CMAKE_INSTALL_WEBOS_PKGDIR} PATTERN ".*" EXCLUDE PATTERN "*.in" EXCLUDE)
    # Copy manifest
    configure_file(deploy/webos/appinfo.json.in ${CMAKE_INSTALL_WEBOS_PKGDIR}/appinfo.json @ONLY)

    add_custom_target(webos-generate-gamecontrollerdb
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/webos/gen_gamecontrollerdb.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    set(WEBOS_PACKAGE_FILENAME ${WEBOS_APPINFO_ID}_${PROJECT_VERSION}_$ENV{ARCH}.ipk)

    add_custom_target(webos-package-moonlight
        COMMAND ${CMAKE_COMMAND} --build . --target install
        COMMAND ares-package ${CMAKE_INSTALL_WEBOS_PKGDIR} -e mdns
    )

    set_target_properties(moonlight PROPERTIES
        WEBOS_PACKAGE_TARGET webos-package-moonlight
        WEBOS_PACKAGE_PATH ${CMAKE_BINARY_DIR}/${WEBOS_PACKAGE_FILENAME}
        WEBOS_PACKAGE_FILENAME ${WEBOS_PACKAGE_FILENAME}
        WEBOS_APPINFO_ID ${WEBOS_APPINFO_ID}
    )
    
    if ($ENV{CI})
        get_target_property(package_id moonlight WEBOS_APPINFO_ID)
        get_target_property(package_file moonlight WEBOS_PACKAGE_PATH)
        get_target_property(package_filename moonlight WEBOS_PACKAGE_FILENAME)
        get_target_property(package_target moonlight WEBOS_PACKAGE_TARGET)

        set(WEBOS_HOMEBREW_MANIFEST_FILENAME "${package_id}.manifest.json")

        add_custom_command(TARGET ${package_target} POST_BUILD
            COMMAND echo RELEASE_IPK_FILENAME=${package_filename} >> ${CMAKE_CURRENT_BINARY_DIR}/github-release.env
            # Force PATH environment variable to use system python
            COMMAND PATH=/bin:/usr/bin ${CMAKE_SOURCE_DIR}/scripts/tools/gen_manifest.py -a ${CMAKE_INSTALL_WEBOS_PKGDIR}/appinfo.json 
                -p ${CMAKE_BINARY_DIR}/${WEBOS_PACKAGE_FILENAME}
                -o ${CMAKE_BINARY_DIR}/${WEBOS_HOMEBREW_MANIFEST_FILENAME}
            COMMAND echo RELEASE_MANIFEST_FILENAME=${WEBOS_HOMEBREW_MANIFEST_FILENAME} >> ${CMAKE_CURRENT_BINARY_DIR}/github-release.env
        )
    else()
        include(WebOSInstall)
        target_webos_install(moonlight)
    endif()

    # add_dependencies(webos-package-moonlight webos-generate-gamecontrollerdb)
endif()
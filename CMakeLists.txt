cmake_minimum_required(VERSION 3.6)
project(moonlight
        VERSION 1.1.5
        LANGUAGES C
        )
# To suppress warnings for MbedTLS
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH
        ${CMAKE_SOURCE_DIR}/utils/cmake
        ${CMAKE_SOURCE_DIR}/cmake
        ${CMAKE_SOURCE_DIR}/core/moonlight-common-c/cmake
        )
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(CC_BASENAME ${CMAKE_C_COMPILER} NAME)

option(TARGET_DESKTOP "Build for Desktop" OFF)
option(TARGET_WEBOS "Build for LG webOS" OFF)
option(TARGET_RASPI "Build for Raspberry Pi" OFF)

option(OPUS_INSTALL_CMAKE_CONFIG_MODULE "" OFF)
option(OPUS_INSTALL_PKG_CONFIG_MODULE "" OFF)

option(USE_MBEDTLS "" ON)
option(USE_WERROR "" OFF)

option(ENABLE_PROGRAMS "Skip mbedtls programs" OFF)
option(ENABLE_TESTING "Skip mbedtls testing" OFF)
option(INSTALL_MBEDTLS_HEADERS "Don't install mbedtls headers" OFF)
option(MBEDTLS_FATAL_WARNINGS "Turn mbedtls warning as errors off" OFF)

option(BUILD_EXAMPLES "" OFF)
option(BUILD_TESTS "" OFF)

set(FEATURE_FORCE_FULLSCREEN OFF)
set(FEATURE_I18N_GETTEXT ON)

include(LintOptions)

if (TARGET_WEBOS)
    if (NOT ENV{ARCH})
        string(REPLACE "-" ";" CC_BASENAME_SEGS ${CC_BASENAME})
        list(GET CC_BASENAME_SEGS 0 CC_ARCH)
        if (CC_ARCH MATCHES [[i.86]])
            set(ENV{ARCH} "x86")
            set(ENV{PKG_CONFIG_PATH} "/usr/lib/i386-linux-gnu/pkgconfig")
        else ()
            set(ENV{ARCH} ${CC_ARCH})
        endif ()
    endif ()
    set(THREADS_PTHREAD_ARG "2" CACHE STRING "Result from TRY_RUN" FORCE)
    set(HAVE_SDL TRUE)

    set(WEBOS_APPINFO_ID "com.limelight.webos")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/webos-unused-prefix)
    set(CMAKE_INSTALL_WEBOS_PKGDIR ${CMAKE_BINARY_DIR}/webos_pkg_$ENV{ARCH})
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_WEBOS_PKGDIR}/lib)
    set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_WEBOS_PKGDIR})
    set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_WEBOS_PKGDIR})
elseif (TARGET_RASPI)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/vc/lib/pkgconfig/")
    set(TARGET_LINUX TRUE)
    set(HAVE_SDL TRUE)

    set(CMAKE_INSTALL_PREFIX /usr)
    include(GNUInstallDirs)
    set(CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR}/moonlight-tv)
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
elseif (TARGET_DESKTOP)
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(TARGET_DARWIN TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(TARGET_LINUX TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(TARGET_WINDOWS TRUE)
    else ()
        message(FATAL_ERROR "Unknown build system")
    endif ()

    set(HAVE_SDL TRUE)

    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
    set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_PREFIX}/share)
else ()
    message(FATAL_ERROR "Unknown build platform")
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

find_package(Config REQUIRED)
find_package(CURL REQUIRED)
find_package(EXPAT REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)

set(BUILD_SHARED_CORE_LIBS OFF)

if (TARGET_WEBOS)
    set(BUILD_SHARED_LIBS ON)
    add_subdirectory(third_party/opus)
    unset(BUILD_SHARED_LIBS)

    add_subdirectory(third_party/mbedtls)
    add_subdirectory(system/webos/tv-native-apis EXCLUDE_FROM_ALL)
    add_subdirectory(system/webos/prebuilts)

    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(PBNJSON REQUIRED pbnjson_c)
    pkg_check_modules(PMLOG REQUIRED PmLogLib)
    pkg_check_modules(WEBOSI18N REQUIRED webosi18n)
elseif (TARGET_RASPI)
    add_subdirectory(third_party/h264bitstream)

    find_package(BroadcomMMAL REQUIRED)
    find_package(MbedTLS REQUIRED)

    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(OPUS REQUIRED opus)

    set(BUILD_SHARED_CORE_LIBS ON)
elseif (TARGET_DESKTOP)
    find_package(MbedTLS REQUIRED)

    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(OPUS REQUIRED opus)
else ()
    message(FATAL_ERROR "Unknown build platform")
endif ()

set(BUILD_SHARED_LIBS ${BUILD_SHARED_CORE_LIBS})
add_subdirectory(core/moonlight-common-c)
add_subdirectory(core/libgamestream)
if (BUILD_SHARED_LIBS)
    install(TARGETS moonlight-common-c LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()
unset(BUILD_SHARED_LIBS)

if (TARGET_DESKTOP)
    set(DECODER_FFMPEG_STATIC ON)
endif ()
add_subdirectory(modules/decoder)
add_subdirectory(modules/audio)
add_subdirectory(third_party/lvgl EXCLUDE_FROM_ALL)
target_include_directories(lvgl PUBLIC ${CMAKE_SOURCE_DIR}/third_party/lvgl/src)
target_include_directories(lvgl PRIVATE ${CMAKE_SOURCE_DIR}/app/util)
target_include_directories(lvgl SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(lvgl SYSTEM PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(lvgl PUBLIC LV_CONF_PATH=../../../app/lvgl/lv_conf.h)

set(SOURCES
        app/main.c
        app/backend/backend_root.c
        app/backend/pcmanager/pcmanager.c
        app/backend/pcmanager/pairing.c
        app/backend/pcmanager/wol.c
        app/backend/pcmanager/pcmanager_common.c
        app/backend/pcmanager/known_hosts.c
        app/backend/pcmanager/discovery.c
        app/backend/pcmanager/pclist.c
        app/backend/pcmanager/listeners.c
        app/backend/gamecontrollerdb_updater.c
        app/backend/apploader/apploader.c
        app/stream/module/module.c
        app/stream/module/audio.c
        app/stream/module/decoder.c
        app/stream/module/symbols.c
        app/stream/session/session.c
        app/stream/session/connection.c
        app/stream/settings.c
        app/stream/video/delegate.c
        app/stream/video/dummy.c
        app/ui/root.c
        app/ui/launcher/launcher.view.c
        app/ui/launcher/launcher.controller.c
        app/ui/launcher/apps.controller.c
        app/ui/launcher/add.dialog.c
        app/ui/launcher/pair.dialog.c
        app/ui/launcher/appitem.view.c
        app/ui/launcher/server.context_menu.c
        app/ui/launcher/coverloader.c
        app/ui/streaming/streaming.view.c
        app/ui/streaming/streaming.controller.c
        app/ui/settings/settings.controller.c
        app/ui/settings/settings.view.c
        app/ui/settings/panes/basic.pane.c
        app/ui/settings/panes/host.pane.c
        app/ui/settings/panes/input.pane.c
        app/ui/settings/panes/decoder.pane.c
        app/ui/settings/panes/about.pane.c
        app/ui/settings/panes/pref_obj.c
        app/lvgl/lv_disp_drv_app.c
        app/lvgl/lv_sdl_drv_key_input.c
        app/lvgl/lv_sdl_drv_pointer_input.c
        app/lvgl/lv_sdl_drv_wheel_input.c
        app/lvgl/lv_ext_utils.c
        app/lvgl/lv_sdl_img.c
        app/lvgl/ext/lv_gridview.c
        app/lvgl/ext/lv_obj_controller.c
        app/lvgl/ext/lv_child_group.c
        app/lvgl/util/lv_app_utils.c
        app/lvgl/font/material_icon_regular_28.c
        app/lvgl/font/material_icon_regular_32.c
        app/lvgl/theme/lv_theme_moonlight.c
        app/util/path.c
        app/util/debugprint.c
        app/util/logging.c
        app/util/libconfig_ext.c
        app/util/img_loader.c
        app/util/nullable.c
        app/util/font.c
        app/app_sdl.c
        app/stream/input/sdlinput.c
        app/stream/input/sdl/keyboard.c
        app/stream/input/sdl/gamepad.c
        app/stream/input/sdl/mouse.c
        app/platform/sdl/bus.c
        app/platform/sdl/input_manager.c
        app/platform/sdl/navkey_sdl.c
        app/stream/audio/sdlaud.c)

if (TARGET_WEBOS)
    list(APPEND SOURCES
            app/stream/input/sdl/keyboard_webos.c
            app/platform/webos/navkey_sdl_webos.c
            app/platform/webos/path_webos.c
            app/platform/webos/os_info.c
            app/platform/webos/SDL_webOS_compat.c
            app/platform/webos/logging_pmlog.c
            app/platform/webos/app_webos.c
            app/platform/webos/lunasynccall.c
            app/platform/webos/i18n_webos.c
            )
else ()
    list(APPEND SOURCES
            app/platform/sdl/path_sdl.c
            app/platform/linux/os_info_linux.c
            app/platform/common/logging_stdio.c
            app/platform/common/app_common.c
            app/platform/common/i18n_gettext.c
            )
endif ()

if (NOT HAVE_SDL)
    message(FATAL_ERROR "SDL2 required")
endif ()

add_executable(moonlight ${SOURCES})
if (WITH_SANITIZE)
    target_compile_options(moonlight PRIVATE -fsanitize=${WITH_SANITIZE} -g3 -O0)
    target_link_options(moonlight PRIVATE -fsanitize=${WITH_SANITIZE} -static-libasan)
endif ()

target_compile_lint(moonlight)

set_target_properties(moonlight PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED TRUE
        )

target_compile_definitions(moonlight PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
string(LENGTH ${CMAKE_SOURCE_DIR} SOURCE_DIR_LENGTH)
target_compile_definitions(moonlight PRIVATE SOURCE_DIR_LENGTH=${SOURCE_DIR_LENGTH})
target_compile_definitions(moonlight PRIVATE APP_VERSION="${PROJECT_VERSION}" APPLOG_HOST=1)
target_compile_definitions(moonlight PRIVATE LV_LVGL_H_INCLUDE_SIMPLE=1)

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/core/libgamestream)
target_include_directories(moonlight SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/incbin)

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/utils/include)

if (TARGET_WINDOWS)
    pkg_check_modules(MICRODNS REQUIRED microdns)
    target_link_libraries(moonlight PRIVATE ${MICRODNS_LIBRARIES})
else ()
    include(BuildMicrodns)
    target_link_libraries(moonlight PRIVATE microdns)
endif ()

target_include_directories(moonlight SYSTEM PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(moonlight PRIVATE
        m ${CMAKE_DL_LIBS} Threads::Threads
        moonlight-common-c gamestream lvgl
        Fontconfig::Fontconfig Config::Config ${CURL_LIBRARIES} ${FREETYPE_LIBRARIES}
        )

macro(moonlight_add_module)
    set(multiValueArgs TARGETS)
    set(oneValueArgs UNUSED)
    set(options LINK)
    cmake_parse_arguments(MOONLIGHT_ADD_MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    foreach (module ${MOONLIGHT_ADD_MODULE_TARGETS})
        if (NOT TARGET ${module})
            continue()
        endif ()
        target_compile_lint(${module})
        if (${MOONLIGHT_ADD_MODULE_LINK})
            target_link_libraries(moonlight PRIVATE ${module})
        else ()
            add_dependencies(moonlight ${module})
        endif ()
    endforeach ()
endmacro()

if (TARGET_WEBOS)
    set(FEATURE_FORCE_FULLSCREEN ON)
    set(FEATURE_I18N_GETTEXT OFF)
    target_compile_definitions(moonlight PUBLIC TARGET_WEBOS OS_LINUX _GNU_SOURCE APPID="${WEBOS_APPINFO_ID}")
    target_compile_definitions(moonlight PUBLIC HAVE_SDL HAVE_RELATIVE_MOUSE_HACK)
    target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="webOS" GAMECONTROLLERDB_PLATFORM_USE="Linux")

    moonlight_add_module(TARGETS moonlight-smp moonlight-smp-webos4 moonlight-smp-webos3)
    moonlight_add_module(TARGETS moonlight-dile-legacy)
    moonlight_add_module(TARGETS moonlight-ndl-webos5 moonlight-ndl)
    moonlight_add_module(TARGETS moonlight-lgnc)
    moonlight_add_module(TARGETS moonlight-pulse)
    moonlight_add_module(TARGETS moonlight-ndlaud moonlight-ndlaud-webos5)

    target_link_libraries(moonlight PRIVATE opus)
    target_link_libraries(moonlight PRIVATE mbedtls)

    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

    target_include_directories(moonlight SYSTEM PRIVATE ${PBNJSON_INCLUDE_DIRS} ${PMLOG_INCLUDE_DIRS}
            ${WEBOSI18N_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${PBNJSON_LIBRARIES} ${PMLOG_LIBRARIES} ${WEBOSI18N_LIBRARIES} helpers)
elseif (TARGET_RASPI)
    target_compile_definitions(moonlight PUBLIC TARGET_RASPI OS_LINUX _GNU_SOURCE)
    target_compile_definitions(moonlight PUBLIC HAVE_SDL)
    target_compile_definitions(moonlight PUBLIC HAVE_PI HAVE_MMAL)
    target_compile_definitions(moonlight PUBLIC HAVE_SURROUND_SOUND)
    target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Linux")

    target_link_libraries(moonlight PRIVATE ${MBEDCRYPTO_LIBRARY})
    target_include_directories(moonlight SYSTEM PRIVATE ${MBEDTLS_INCLUDE_DIRS})

    moonlight_add_module(TARGETS moonlight-mmal LINK)
    moonlight_add_module(TARGETS moonlight-ffmpeg LINK)
    moonlight_add_module(TARGETS moonlight-alsa LINK)
    moonlight_add_module(TARGETS moonlight-pulse LINK)

    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

    target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})
    set_target_properties(moonlight PROPERTIES OUTPUT_NAME moonlight-tv)
elseif (TARGET_DESKTOP)
    target_compile_definitions(moonlight PUBLIC TARGET_DESKTOP)

    if (TARGET_DARWIN)
        target_compile_definitions(moonlight PUBLIC OS_DARWIN)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Mac OS X")
    elseif (TARGET_LINUX)
        target_compile_definitions(moonlight PUBLIC OS_LINUX _GNU_SOURCE)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Linux")
    elseif (TARGET_WINDOWS)
        target_compile_definitions(moonlight PUBLIC OS_WINDOWS TARGET_WINDOWS)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Windows")
    endif ()

    target_compile_definitions(moonlight PUBLIC HAVE_SDL)

    target_link_libraries(moonlight PRIVATE mbedtls)

    target_link_libraries(moonlight PRIVATE moonlight-ffmpeg)

    target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})

    target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
else ()
    message(FATAL_ERROR "Unknown build platform")
endif ()

if (FEATURE_FORCE_FULLSCREEN)
    target_compile_definitions(moonlight PRIVATE FEATURE_FORCE_FULLSCREEN=1)
else()
    target_compile_definitions(moonlight PRIVATE FEATURE_FORCE_FULLSCREEN=0)
endif()

if (TARGET_WINDOWS)
    target_link_libraries(moonlight PRIVATE rpcrt4)
else ()
    pkg_check_modules(UUID REQUIRED uuid)
    target_link_libraries(moonlight PRIVATE ${UUID_LIBRARIES})
endif ()

install(TARGETS moonlight DESTINATION ${CMAKE_INSTALL_BINDIR})

set(I18N_LOCALES "ja")

if (FEATURE_I18N_GETTEXT)
    add_custom_command(TARGET moonlight POST_BUILD
            COMMAND mkdir -p ${CMAKE_BINARY_DIR}/mo/ja/LC_MESSAGES
            COMMAND msgfmt --output-file=${CMAKE_BINARY_DIR}/mo/ja/LC_MESSAGES/moonlight-tv.mo i18n/ja/messages.po
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            VERBATIM)
endif ()

add_custom_target(i18n-update-pot
        COMMAND xgettext --keyword=locstr --keyword=translatable --add-comments --sort-output -o i18n/messages.pot
        ${SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(i18n-merge-po
        COMMAND msgmerge --update i18n/ja/messages.po i18n/messages.pot
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


if (TARGET_WEBOS)
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
            COMMAND npm run webos-gen-i18n -- -o ${CMAKE_INSTALL_WEBOS_PKGDIR}/resources ${I18N_LOCALES}
            COMMAND ares-package ${CMAKE_INSTALL_WEBOS_PKGDIR} -e include -e cmake -e "libconfig\\\\+\\\\+\\\\..*"
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
    else ()
        include(WebOSInstall)
        target_webos_install(moonlight)
    endif ()

    # add_dependencies(webos-package-moonlight webos-generate-gamecontrollerdb)
elseif (TARGET_RASPI)
    install(FILES deploy/linux/moonlight-tv.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES deploy/linux/moonlight-tv.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/mo/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/locale)

    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "moonlight-tv")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
    set(CPACK_DEBIAN_PACKAGE_SUMMARY "Open Source NVIDIA GameStream Client")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mariotaku Lee <mariotaku.lee@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/mariotaku/moonlight-tv")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    # set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    # set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS_LIST
            "libc6 (>= 2.28)"
            "libsdl2-2.0-0 (>= 2.0.4)"
            "libsdl2-image-2.0-0 (>= 2.0.1)"
            "libopus0 (>= 1.1)"
            "libcurl4 (>= 7.16.2)"
            "libuuid1 (>= 2.16)"
            "libexpat1 (>= 2.0.1)"
            "libmbedcrypto3 (>= 2.13)"
            "libmbedx509-0 (>= 2.0)"
            "libconfig9 (>= 1.5)"
            "libraspberrypi0"
            )
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS_LIST
            "libasound2 (>= 1.0.16)"
            "libpulse0 (>= 0.99.1)"
            )
    set(CPACK_DEBIAN_PACKAGE_SUGGESTS_LIST
            "libavcodec58 (>= 7:4.0)"
            "libavutil56 (>= 7:4.0)"
            )
    string(REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS_LIST}")
    string(REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_RECOMMENDS "${CPACK_DEBIAN_PACKAGE_RECOMMENDS_LIST}")
    string(REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_SUGGESTS "${CPACK_DEBIAN_PACKAGE_SUGGESTS_LIST}")

    set(CPACK_PACKAGE_FILE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-${CPACK_DEBIAN_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

    include(CPack)
endif ()
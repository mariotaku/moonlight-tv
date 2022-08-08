cmake_minimum_required(VERSION 3.13)
project(moonlight
        VERSION 1.4.8
        LANGUAGES C
        )
# To suppress warnings for MbedTLS
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH
        ${CMAKE_SOURCE_DIR}/system/webos/utils/cmake
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
option(USE_STATIC_MBEDTLS_LIBRARY "Don't build mbed TLS static library." OFF)
option(USE_SHARED_MBEDTLS_LIBRARY "Build mbed TLS shared library." ON)

option(BUILD_EXAMPLES "" OFF)
option(BUILD_TESTS "" ON)

set(I18N_LOCALES "cs" "de" "fr" "it" "nl" "pl" "pt-BR" "ro" "ja" "zh-CN")
list(LENGTH I18N_LOCALES I18N_LOCALES_LEN)

set(FEATURE_FORCE_FULLSCREEN OFF)
set(FEATURE_I18N_GETTEXT ON)
set(FEATURE_I18N_LANGUAGE_SETTINGS OFF)
set(FEATURE_SURROUND_SOUND ON)
set(FEATURE_CHECK_MODULE_OS_VERSION OFF)

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
elseif (TARGET_RASPI)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/vc/lib/pkgconfig/")
    set(OS_LINUX TRUE)
    set(HAVE_SDL TRUE)

    set(CMAKE_INSTALL_PREFIX /usr)
    include(GNUInstallDirs)
    set(CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR}/moonlight-tv)
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
elseif (TARGET_DESKTOP)
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(OS_DARWIN TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(OS_LINUX TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OS_WINDOWS TRUE)
    else ()
        message(FATAL_ERROR "Unknown build system")
    endif ()

    set(HAVE_SDL TRUE)

    set(CMAKE_INSTALL_LIBDIR lib)
    set(CMAKE_INSTALL_BINDIR bin)
    set(CMAKE_INSTALL_DATAROOTDIR share)
else ()
    message(FATAL_ERROR "Unknown build platform")
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

find_package(EXPAT REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)

find_program(NPM_COMMAND npm REQUIRED)

execute_process(COMMAND ${NPM_COMMAND} install --quiet)

set(BUILD_SHARED_CORE_LIBS OFF)

pkg_check_modules(SDL2 REQUIRED sdl2 SDL2_image)

if (TARGET_WEBOS)
    set(BUILD_SHARED_LIBS ON)
    add_subdirectory(third_party/opus)
    add_subdirectory(third_party/mbedtls)
    unset(BUILD_SHARED_LIBS)

    add_subdirectory(system/webos/tv-native-apis)
    set(CURL_LIBRARIES curl-shim)

    pkg_check_modules(PBNJSON_C REQUIRED pbnjson_c)
    pkg_check_modules(PMLOG REQUIRED PmLogLib)
    pkg_check_modules(WEBOSI18N REQUIRED webosi18n)
elseif (TARGET_RASPI)
    add_subdirectory(third_party/h264bitstream)

    find_package(CURL REQUIRED)
    find_package(MbedTLS REQUIRED)

    pkg_check_modules(OPUS REQUIRED opus)
    pkg_check_modules(BCM_HOST REQUIRED bcm_host)
    pkg_check_modules(MMAL REQUIRED mmal)
    pkg_check_modules(VCSM REQUIRED vcsm)

    set(BUILD_SHARED_CORE_LIBS ON)
elseif (TARGET_DESKTOP)
    find_package(CURL REQUIRED)
    find_package(MbedTLS REQUIRED)

    pkg_check_modules(OPUS REQUIRED opus)
else ()
    message(FATAL_ERROR "Unknown build platform")
endif ()

set(BUILD_SHARED_LIBS ${BUILD_SHARED_CORE_LIBS})
add_subdirectory(core/moonlight-common-c)
add_subdirectory(core/libgamestream)
if (BUILD_SHARED_LIBS)
    install(TARGETS moonlight-common-c LIBRARY DESTINATION lib)
endif ()
unset(BUILD_SHARED_LIBS)
include(BuildINIH)

if (TARGET_DESKTOP)
    set(DECODER_FFMPEG_STATIC ON)
endif ()
add_subdirectory(modules)
add_subdirectory(third_party/lvgl EXCLUDE_FROM_ALL)
target_include_directories(lvgl PUBLIC ${CMAKE_SOURCE_DIR}/third_party/lvgl/src)
target_include_directories(lvgl PRIVATE ${CMAKE_SOURCE_DIR}/app/util)
target_include_directories(lvgl SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(lvgl SYSTEM PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(lvgl PUBLIC LV_CONF_PATH=../../../app/lvgl/lv_conf.h)

set(SOURCES
        app/main.c
        app/res.c
        app/app/app_input.c
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
        app/stream/session/watchdog.c
        app/stream/settings.c
        app/stream/audio/stats.c
        app/stream/video/delegate.c
        app/stream/video/dummy.c
        app/stream/video/sps_parser.c
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
        app/ui/help/help.dialog.c
        app/ui/common/progress_dialog.c
        app/lvgl/lv_disp_drv_app.c
        app/lvgl/lv_sdl_drv_key_input.c
        app/lvgl/lv_sdl_drv_pointer_input.c
        app/lvgl/lv_sdl_drv_wheel_input.c
        app/lvgl/lv_sdl_drv_button_input.c
        app/lvgl/lv_ext_utils.c
        app/lvgl/lv_sdl_img.c
        app/lvgl/ext/lv_gridview.c
        app/lvgl/ext/lv_child_group.c
        app/lvgl/util/lv_app_utils.c
        app/lvgl/font/font_empty.c
        app/lvgl/theme/lv_theme_moonlight.c
        app/util/path.c
        app/util/debugprint.c
        app/util/logging.c
        app/util/img_loader.c
        app/util/nullable.c
        app/util/font.c
        app/util/ini_ext.c
        app/app_sdl.c
        app/stream/input/sdlinput.c
        app/stream/input/sdl/keyboard.c
        app/stream/input/sdl/gamepad.c
        app/stream/input/sdl/mouse.c
        app/platform/sdl/bus.c
        app/platform/sdl/input_manager.c
        app/platform/sdl/navkey_sdl.c
        app/platform/common/i18n_common.c)

if (TARGET_WEBOS)
    list(APPEND SOURCES
            app/platform/webos/keyboard_webos.c
            app/platform/webos/navkey_sdl_webos.c
            app/platform/webos/path_webos.c
            app/platform/webos/os_info.c
            app/platform/webos/SDL_webOS_compat.c
            app/platform/webos/pbnjson_compat.c
            app/platform/webos/logging_pmlog.c
            app/platform/webos/app_webos.c
            app/platform/webos/lunasynccall.c
            app/platform/webos/i18n_webos.c
            )
else ()
    list(APPEND SOURCES
            app/platform/sdl/path_sdl.c
            app/platform/common/logging_stdio.c
            app/platform/common/app_common.c
            app/platform/common/i18n_gettext.c
            )
    if (OS_WINDOWS)
        list(APPEND SOURCES app/platform/windows/os_info_windows.c)
    else ()
        list(APPEND SOURCES app/platform/linux/os_info_linux.c)
    endif ()
endif ()

set(RES
        res/moonlight.png
        res/defcover.png
        res/fav_indicator.png
        app/lvgl/font/material_icons_regular.ttf)


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

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/utils/include)

if (OS_WINDOWS)
    pkg_check_modules(MICRODNS REQUIRED microdns)
    target_link_libraries(moonlight PRIVATE ${MICRODNS_LIBRARIES})
    if (MINGW)
        target_link_libraries(moonlight PRIVATE libintl.a libiconv.a)
    endif ()
else ()
    set(BUILD_SHARED_LIBS ON)
    include(BuildMicrodns)
    target_link_libraries(moonlight PRIVATE microdns)
    unset(BUILD_SHARED_LIBS)
endif ()

target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(moonlight PRIVATE
        m ${CMAKE_DL_LIBS} Threads::Threads ${SDL2_LIBRARIES}
        moonlight-common-c gamestream lvgl inih ${CURL_LIBRARIES}
        Fontconfig::Fontconfig ${FREETYPE_LIBRARIES}
        moonlight-module-api moonlight-sdlaud
        )

add_custom_target(moonlight-res
        COMMAND ${NPM_COMMAND} run gen-res-headers -- -i ${RES} -o app/res
        SOURCES ${RES})
add_dependencies(moonlight moonlight-res)

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
    set(FEATURE_I18N_LANGUAGE_SETTINGS ON)
    set(FEATURE_CHECK_MODULE_OS_VERSION ON)

    target_compile_definitions(moonlight PUBLIC TARGET_WEBOS OS_LINUX _GNU_SOURCE APPID="${WEBOS_APPINFO_ID}")
    target_compile_definitions(moonlight PUBLIC HAVE_RELATIVE_MOUSE_HACK)
    target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="webOS" GAMECONTROLLERDB_PLATFORM_USE="Linux")

    moonlight_add_module(TARGETS moonlight-smp moonlight-smp-webos4 moonlight-smp-webos3)
    moonlight_add_module(TARGETS moonlight-dile-legacy)
    moonlight_add_module(TARGETS moonlight-ndl-webos5 moonlight-ndl)
    moonlight_add_module(TARGETS moonlight-cgl moonlight-lgnc)
    moonlight_add_module(TARGETS moonlight-pulse)
    moonlight_add_module(TARGETS moonlight-ndlaud moonlight-ndlaud-webos5)

    target_link_libraries(moonlight PRIVATE opus)
    target_link_libraries(moonlight PRIVATE mbedcrypto mbedx509)

    target_include_directories(moonlight SYSTEM PRIVATE ${PBNJSON_C_INCLUDE_DIRS} ${PMLOG_INCLUDE_DIRS}
            ${WEBOSI18N_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${PBNJSON_C_LIBRARIES} ${PMLOG_LIBRARIES} ${WEBOSI18N_LIBRARIES} helpers)
elseif (TARGET_RASPI)
    target_compile_definitions(moonlight PUBLIC TARGET_RASPI OS_LINUX _GNU_SOURCE)
    target_compile_definitions(moonlight PUBLIC HAVE_PI HAVE_MMAL)
    target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Linux")

    target_include_directories(moonlight SYSTEM PRIVATE ${MBEDTLS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${MBEDCRYPTO_LIBRARY} ${MBEDX509_LIBRARY})

    moonlight_add_module(TARGETS moonlight-mmal LINK)
    moonlight_add_module(TARGETS moonlight-ffmpeg LINK)
    moonlight_add_module(TARGETS moonlight-alsa LINK)
    moonlight_add_module(TARGETS moonlight-pulse LINK)

    target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})

    set_target_properties(moonlight PROPERTIES OUTPUT_NAME moonlight-tv)
elseif (TARGET_DESKTOP)
    target_compile_definitions(moonlight PUBLIC TARGET_DESKTOP)

    if (OS_DARWIN)
        target_compile_definitions(moonlight PUBLIC OS_DARWIN)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Mac OS X")
    elseif (OS_LINUX)
        target_compile_definitions(moonlight PUBLIC OS_LINUX _GNU_SOURCE)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Linux")
    elseif (OS_WINDOWS)
        target_compile_definitions(moonlight PUBLIC OS_WINDOWS)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Windows")
    endif ()

    target_include_directories(moonlight SYSTEM PRIVATE ${MBEDTLS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${MBEDCRYPTO_LIBRARY})

    target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})

    target_link_libraries(moonlight PRIVATE moonlight-ffmpeg)
else ()
    message(FATAL_ERROR "Unknown build platform")
endif ()

configure_file(app/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
target_include_directories(moonlight PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (OS_WINDOWS)
    target_link_libraries(moonlight PRIVATE rpcrt4)
else ()
    pkg_check_modules(UUID REQUIRED uuid)
    target_link_libraries(moonlight PRIVATE ${UUID_LIBRARIES})
endif ()

if (BUILD_TESTS)
    enable_testing()

    add_subdirectory(third_party/Unity EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif ()

install(TARGETS moonlight RUNTIME DESTINATION bin)


if (FEATURE_I18N_GETTEXT)
    find_program(MSGFMT_COMMAND msgfmt)
    if (NOT MSGFMT_COMMAND)
        message(FATAL_ERROR "msgfmt not found. Please install gettext.")
    endif ()
    set(MOFILES)
    foreach (LANG ${I18N_LOCALES})
        string(REPLACE "-" "_" _MOLANG ${LANG})
        set(_POFILE ${CMAKE_SOURCE_DIR}/i18n/${LANG}/messages.po)
        set(_MODIR ${CMAKE_BINARY_DIR}/mo/${_MOLANG}/LC_MESSAGES)
        set(_MOFILE ${_MODIR}/moonlight-tv.mo)
        add_custom_command(OUTPUT ${_MOFILE} DEPENDS i18n/${LANG}/messages.po
                COMMAND ${CMAKE_COMMAND} -E make_directory ${_MODIR}
                COMMAND ${MSGFMT_COMMAND} --output-file=${_MOFILE} ${_POFILE})
        list(APPEND MOFILES ${_MOFILE})
    endforeach ()
    add_custom_target(moonlight-i18n ALL DEPENDS ${MOFILES})
    add_dependencies(moonlight moonlight-i18n)
endif ()

find_program(XGETTEXT_COMMAND xgettext)
find_program(MSGMERGE_COMMAND msgmerge)

add_custom_target(i18n-update-pot
        COMMAND ${XGETTEXT_COMMAND} --keyword=locstr --keyword=translatable --add-comments -o i18n/messages.pot
        ${SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(i18n-merge-po
        COMMAND ${MSGMERGE_COMMAND} --update i18n/ja/messages.po i18n/messages.pot
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(iconfont-minify
        COMMAND ${NPM_COMMAND} run iconfont-minify -- -f res/MaterialIcons-Regular.ttf -c res/MaterialIcons-Regular.codepoints
        -l res/mat_icons_font.list -o app/lvgl/font -n material_icons_regular -p MAT_SYMBOL
        )

if (TARGET_WEBOS)
    include(PackageWebOS)
    if (NOT ENV{CI})
        include(WebOSInstall)
        target_webos_install(moonlight)
    endif ()
elseif (TARGET_RASPI)
    include(PackageRaspbian)
elseif (OS_WINDOWS)
endif ()
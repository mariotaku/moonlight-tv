cmake_minimum_required(VERSION 3.13)
project(moonlight
        VERSION 1.5.7
        LANGUAGES C
        )
# To suppress warnings for MbedTLS
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif ()
# To suppress warnings for ExternalProject DOWNLOAD_EXTRACT_TIMESTAMP
if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH
        ${CMAKE_SOURCE_DIR}/system/webos/utils/cmake
        ${CMAKE_SOURCE_DIR}/cmake
        ${CMAKE_SOURCE_DIR}/third_party/commons/cmake
        ${CMAKE_SOURCE_DIR}/core/moonlight-common-c/cmake
        )
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(CC_BASENAME ${CMAKE_C_COMPILER} NAME)

option(TARGET_DESKTOP "Build for Desktop" OFF)
option(TARGET_WEBOS "Build for LG webOS" OFF)
option(TARGET_RASPI "Build for Raspberry Pi" OFF)

option(USE_MBEDTLS "" ON)
option(USE_WERROR "" OFF)

option(BUILD_EXAMPLES "" OFF)

option(COMMONS_LV_SDL_IMG_USE_IMAGE "Use SDL2_image API" ON)

set(COMMONS_LOGGING_SDL ON)
set(COMMONS_LOGGING_LVGL ON)
set(COMMONS_LOGGING_SS4S ON)

if (TARGET_WEBOS)
    option(BUILD_TESTS "" OFF)
else ()
    option(BUILD_TESTS "" ON)
endif ()

set(I18N_LOCALES "cs" "de" "fr" "it" "nl" "pl" "pt-BR" "ro" "ru" "ja" "zh-CN")
list(LENGTH I18N_LOCALES I18N_LOCALES_LEN)

set(FEATURE_FORCE_FULLSCREEN OFF)
set(FEATURE_I18N_GETTEXT ON)
set(FEATURE_I18N_LANGUAGE_SETTINGS OFF)
set(FEATURE_SURROUND_SOUND ON)
set(FEATURE_CHECK_MODULE_OS_VERSION OFF)
set(FEATURE_INPUT_EVMOUSE OFF)
set(FEATURE_LIBCEC ON)

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
    set(OS_LINUX TRUE)
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
find_package(MbedTLS)

find_program(NPM_COMMAND npm)

if (NPM_COMMAND)
    execute_process(COMMAND ${NPM_COMMAND} install --quiet)
endif ()

set(BUILD_SHARED_CORE_LIBS OFF)

pkg_check_modules(SDL2_AND_IMAGES REQUIRED sdl2 SDL2_image)

pkg_check_modules(OPUS opus)

if (NOT OPUS_FOUND)
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    message(STATUS "Including opus from source...")
    include(ExternalOPUS)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
endif ()

if (NOT MBEDTLS_FOUND)
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    message(STATUS "Including mbedtls from source...")
    include(ExternalMbedTLS)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
endif ()

if (TARGET_WEBOS)

    add_subdirectory(system/webos/tv-native-apis)
    set(CURL_LIBRARIES curl-shim)

    pkg_check_modules(PBNJSON_C REQUIRED pbnjson_c)
    pkg_check_modules(PMLOG REQUIRED PmLogLib)
    pkg_check_modules(WEBOSI18N REQUIRED webosi18n)
else ()
    find_package(CURL REQUIRED)

    set(BUILD_SHARED_CORE_LIBS ON)
endif ()

set(BUILD_SHARED_LIBS ${BUILD_SHARED_CORE_LIBS})
add_subdirectory(core/moonlight-common-c)
add_subdirectory(core/libgamestream)
if (BUILD_SHARED_LIBS)
    install(TARGETS moonlight-common-c LIBRARY DESTINATION lib)
endif ()
unset(BUILD_SHARED_LIBS)
include(BuildINIH)

if (TARGET_WEBOS)
    set(SS4S_MODULE_DISABLE_MMAL ON)
endif ()
option(SS4S_MODULE_BUILD_DUMMY "Enable SS4S dummy module" OFF)

add_subdirectory(third_party/ss4s)
add_subdirectory(third_party/lvgl EXCLUDE_FROM_ALL)
target_include_directories(lvgl PUBLIC ${CMAKE_SOURCE_DIR}/third_party/lvgl/src)
target_include_directories(lvgl PRIVATE ${CMAKE_SOURCE_DIR}/app/util)
target_include_directories(lvgl SYSTEM PRIVATE ${SDL2_AND_IMAGES_INCLUDE_DIRS})
target_include_directories(lvgl SYSTEM PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(lvgl PUBLIC LV_CONF_PATH=../../../app/lvgl/lv_conf.h)

add_subdirectory(third_party/commons EXCLUDE_FROM_ALL)

target_include_directories(lv_gridview SYSTEM PRIVATE ${SDL2_AND_IMAGES_INCLUDE_DIRS})
target_include_directories(lv_sdl_img SYSTEM PRIVATE ${SDL2_AND_IMAGES_INCLUDE_DIRS})

if (NOT HAVE_SDL)
    message(FATAL_ERROR "SDL2 required")
endif ()

add_executable(moonlight)
add_subdirectory(app)
add_subdirectory(res)

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

target_include_directories(moonlight SYSTEM PRIVATE ${SDL2_AND_IMAGES_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(moonlight PRIVATE
        m ${CMAKE_DL_LIBS} Threads::Threads ${SDL2_AND_IMAGES_LIBRARIES}
        moonlight-common-c gamestream lvgl inih ${CURL_LIBRARIES}
        Fontconfig::Fontconfig ${FREETYPE_LIBRARIES})

target_link_libraries(moonlight PRIVATE lv_gridview lv_sdl_img)

target_include_directories(moonlight SYSTEM PRIVATE ${OPUS_INCLUDE_DIRS})
target_link_libraries(moonlight PRIVATE ${OPUS_LIBRARIES})

target_include_directories(moonlight SYSTEM PRIVATE ${MBEDTLS_INCLUDE_DIRS})
target_link_libraries(moonlight PRIVATE ${MBEDCRYPTO_LIBRARY} ${MBEDX509_LIBRARY})

if (TARGET_WEBOS)
    set(FEATURE_FORCE_FULLSCREEN ON)
    set(FEATURE_I18N_GETTEXT OFF)
    set(FEATURE_I18N_LANGUAGE_SETTINGS ON)
    set(FEATURE_CHECK_MODULE_OS_VERSION ON)
    set(FEATURE_LIBCEC OFF)

    target_compile_definitions(moonlight PUBLIC TARGET_WEBOS OS_LINUX _GNU_SOURCE APPID="${WEBOS_APPINFO_ID}")
    target_compile_definitions(moonlight PUBLIC HAVE_RELATIVE_MOUSE_HACK)
    target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="webOS" GAMECONTROLLERDB_PLATFORM_USE="Linux")

    target_link_libraries(moonlight PRIVATE rt)

    target_include_directories(moonlight SYSTEM PRIVATE ${PBNJSON_C_INCLUDE_DIRS} ${PMLOG_INCLUDE_DIRS}
            ${WEBOSI18N_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${PBNJSON_C_LIBRARIES} ${PMLOG_LIBRARIES} ${WEBOSI18N_LIBRARIES} helpers)
    target_link_libraries(moonlight PRIVATE commons-luna-sync)
else ()
    target_compile_definitions(moonlight PUBLIC TARGET_DESKTOP)

    if (OS_DARWIN)
        target_link_directories(moonlight PUBLIC ${SDL2_AND_IMAGES_LIBRARY_DIRS})
        target_compile_definitions(moonlight PUBLIC OS_DARWIN)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Mac OS X")
    elseif (OS_LINUX)
        target_compile_definitions(moonlight PUBLIC OS_LINUX _GNU_SOURCE)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Linux")
    elseif (OS_WINDOWS)
        target_compile_definitions(moonlight PUBLIC OS_WINDOWS)
        target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="Windows")
    else ()
        message(FATAL_ERROR "Unknown build platform")
    endif ()

    set_target_properties(moonlight PROPERTIES OUTPUT_NAME moonlight-tv)
endif ()

if (FEATURE_LIBCEC AND TARGET commons-cec-sdl)
    target_link_libraries(moonlight PRIVATE commons-cec-sdl)
else ()
    set(FEATURE_LIBCEC OFF)
endif ()

if (TARGET_WEBOS)
    set(SS4S_MODULES_INI_PATH_RUNTIME "lib/ss4s_modules.ini")
else ()
    if (SS4S_MODULE_BUILD_DUMMY)
        target_link_libraries(moonlight PRIVATE ss4s ss4s-module-dummy)
    endif ()
    # TODO: following are for development (RPATH and module location), remove for packaged builds
    set(SS4S_MODULES_INI_PATH_RUNTIME ${SS4S_MODULES_INI_OUTPUT_FILE})
endif ()
target_link_libraries(moonlight PRIVATE commons-ss4s-modules-list)
target_compile_definitions(commons-ss4s-modules-list PRIVATE SS4S_MODULES_INI_PATH="${SS4S_MODULES_INI_PATH_RUNTIME}")

target_link_libraries(moonlight PRIVATE commons-logging)

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
    include(MoonlightI18n)
endif ()

if (NPM_COMMAND)
    add_custom_target(iconfont-minify
            COMMAND ${NPM_COMMAND} run iconfont-minify -- -f res/MaterialIcons-Regular.ttf -c res/MaterialIcons-Regular.codepoints
            -l res/mat_icons_font.list -o app/lvgl/font -n material_icons_regular -p MAT_SYMBOL
            )
endif ()

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
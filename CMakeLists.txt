cmake_minimum_required(VERSION 3.19)

execute_process(COMMAND git describe --tags --abbrev=0 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MOONLIGHT_VERSION ERROR_QUIET)
if (MOONLIGHT_VERSION)
    string(STRIP "${MOONLIGHT_VERSION}" MOONLIGHT_VERSION)
    string(SUBSTRING "${MOONLIGHT_VERSION}" 1 -1 MOONLIGHT_VERSION)
else ()
    execute_process(COMMAND curl -s "https://api.github.com/repos/mariotaku/moonlight-tv/releases/latest"
            OUTPUT_VARIABLE MOONLIGHT_LATEST_RELEASE ERROR_QUIET)
    string(JSON MOONLIGHT_VERSION GET "${MOONLIGHT_LATEST_RELEASE}" "tag_name")
    if (MOONLIGHT_VERSION)
        string(REGEX REPLACE "v([0-9]+)" "\\1" MOONLIGHT_VERSION "${MOONLIGHT_VERSION}")
    else ()
        set(MOONLIGHT_VERSION "0.0.0")
    endif ()
endif ()

project(moonlight VERSION ${MOONLIGHT_VERSION} LANGUAGES C)

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

option(TARGET_WEBOS "Build for LG webOS" OFF)

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

if (BUILD_TESTS)
    enable_testing()
endif ()

set(I18N_LOCALES "cs" "de" "fr" "it" "nl" "pl" "pt-BR" "ro" "ru" "ja" "zh-CN")
list(LENGTH I18N_LOCALES I18N_LOCALES_LEN)

set(FEATURE_FORCE_FULLSCREEN OFF)
set(FEATURE_I18N_GETTEXT ON)
set(FEATURE_I18N_LANGUAGE_SETTINGS OFF)
set(FEATURE_SURROUND_SOUND ON)
set(FEATURE_CHECK_MODULE_OS_VERSION OFF)
set(FEATURE_INPUT_EVMOUSE OFF)
set(FEATURE_INPUT_LIBCEC ON)

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

    set(WEBOS_APPINFO_ID "com.limelight.webos")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OS_LINUX TRUE)

    set(CMAKE_INSTALL_PREFIX /usr)
    include(GNUInstallDirs)
    set(CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR}/moonlight-tv)
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
else ()
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(OS_DARWIN TRUE)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OS_WINDOWS TRUE)
    else ()
        message(FATAL_ERROR "Unknown build platform")
    endif ()

    set(CMAKE_INSTALL_LIBDIR lib)
    set(CMAKE_INSTALL_BINDIR bin)
    set(CMAKE_INSTALL_DATAROOTDIR share)
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

find_package(EXPAT REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)
find_package(CURL REQUIRED)
find_package(MbedTLS)

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

    pkg_check_modules(PBNJSON_C REQUIRED pbnjson_c)
    pkg_check_modules(PMLOG REQUIRED PmLogLib)
    pkg_check_modules(WEBOSI18N REQUIRED webosi18n)
else ()

    set(BUILD_SHARED_CORE_LIBS ON)
endif ()

set(BUILD_SHARED_LIBS ${BUILD_SHARED_CORE_LIBS})
add_subdirectory(core/moonlight-common-c)
add_subdirectory(core/libgamestream)
if (BUILD_SHARED_LIBS)
    install(TARGETS moonlight-common-c LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
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
target_link_libraries(lvgl PUBLIC ${SDL2_AND_IMAGES_LIBRARIES} ${FREETYPE_LIBRARIES})

add_subdirectory(third_party/commons)

target_include_directories(lv_gridview SYSTEM PRIVATE ${SDL2_AND_IMAGES_INCLUDE_DIRS})
target_include_directories(lv_sdl_img SYSTEM PRIVATE ${SDL2_AND_IMAGES_INCLUDE_DIRS})

add_executable(moonlight)
add_subdirectory(app)

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
    set(FEATURE_INPUT_LIBCEC OFF)
    set(FEATURE_INPUT_EVMOUSE ON)

    target_compile_definitions(moonlight PUBLIC TARGET_WEBOS OS_LINUX _GNU_SOURCE APPID="${WEBOS_APPINFO_ID}")
    target_compile_definitions(moonlight PUBLIC HAVE_RELATIVE_MOUSE_HACK)
    target_compile_definitions(moonlight PUBLIC GAMECONTROLLERDB_PLATFORM="webOS" GAMECONTROLLERDB_PLATFORM_USE="Linux")

    target_link_libraries(moonlight PRIVATE rt)

    target_include_directories(moonlight SYSTEM PRIVATE ${PBNJSON_C_INCLUDE_DIRS} ${PMLOG_INCLUDE_DIRS}
            ${WEBOSI18N_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${PBNJSON_C_LIBRARIES} ${PMLOG_LIBRARIES} ${WEBOSI18N_LIBRARIES} helpers)
    target_link_libraries(moonlight PRIVATE commons-luna-sync commons-webos-is-uhd commons-webos-panel-info)
else ()
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

if (FEATURE_INPUT_LIBCEC AND TARGET commons-cec-sdl)
    target_link_libraries(moonlight PRIVATE commons-cec-sdl)
else ()
    set(FEATURE_INPUT_LIBCEC OFF)
endif ()

if (FEATURE_INPUT_EVMOUSE AND TARGET commons-evmouse)
    target_link_libraries(moonlight PRIVATE commons-evmouse)
else ()
    set(FEATURE_INPUT_EVMOUSE OFF)
endif ()

if (TARGET_WEBOS)
    set(SS4S_MODULES_INI_PATH_RUNTIME "lib/ss4s_modules.ini")
else ()
    set(SS4S_MODULES_INI_PATH_RUNTIME "${CMAKE_INSTALL_FULL_LIBDIR}/ss4s_modules.ini")
endif ()
target_link_libraries(moonlight PRIVATE commons-ss4s-modules-list)
target_compile_definitions(commons-ss4s-modules-list PRIVATE SS4S_MODULES_INI_PATH="${SS4S_MODULES_INI_PATH_RUNTIME}")

target_link_libraries(moonlight PRIVATE commons-logging commons-gamecontrollerdb-updater commons-lazy
        commons-refcounter commons-executor commons-linked-list)

configure_file(app/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
target_include_directories(moonlight PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (OS_WINDOWS)
    target_link_libraries(moonlight PRIVATE rpcrt4)
else ()
    pkg_check_modules(UUID REQUIRED uuid)
    target_link_libraries(moonlight PRIVATE ${UUID_LIBRARIES})
endif ()

if (BUILD_TESTS)
    add_subdirectory(third_party/Unity EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif ()

install(TARGETS moonlight RUNTIME DESTINATION bin)


if (FEATURE_I18N_GETTEXT)
    include(MoonlightI18n)
endif ()

include(GetCompilerTarget)

if (TARGET_WEBOS)
    include(PackageWebOS)
    if (NOT ENV{CI})
        include(WebOSInstall)
        target_webos_install(moonlight)
    endif ()
elseif (OS_LINUX)
    include(PackageDebian)
elseif (OS_WINDOWS)
endif ()
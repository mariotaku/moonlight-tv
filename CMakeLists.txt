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
        ${CMAKE_SOURCE_DIR}/cmake
        ${CMAKE_SOURCE_DIR}/third_party/commons/cmake
        ${CMAKE_SOURCE_DIR}/core/moonlight-common-c/cmake
        )
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GetCompilerTarget)

get_filename_component(CC_BASENAME ${CMAKE_C_COMPILER} NAME)

if (CMAKE_C_COMPILER_TARGET STREQUAL "arm-webos-linux-gnueabi")
    set(TARGET_WEBOS ON)
    set(TARGET_WEBOS_ARCH "arm")
elseif(CMAKE_C_COMPILER_TARGET MATCHES "(i[3-6]86)-webos-linux-gnu")
    set(TARGET_WEBOS ON)
    set(TARGET_WEBOS_ARCH ${CMAKE_MATCH_1})
elseif (CMAKE_C_COMPILER_TARGET STREQUAL "armv7a-cros-linux-gnueabi")
    set(TARGET_STEAMLINK ON)
endif ()

option(USE_MBEDTLS "" ON)
option(USE_WERROR "" OFF)

option(BUILD_EXAMPLES "" OFF)

option(COMMONS_LV_SDL_IMG_USE_IMAGE "Use SDL2_image API" ON)

set(COMMONS_LOGGING_SDL ON)
set(COMMONS_LOGGING_LVGL ON)
set(COMMONS_LOGGING_SS4S ON)

if (TARGET_WEBOS OR TARGET_STEAMLINK)
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
set(FEATURE_EMBEDDED_SHELL OFF)
set(FEATURE_WINDOW_FULLSCREEN_DESKTOP ON)

include(LintOptions)

if (TARGET_WEBOS)
    set(THREADS_PTHREAD_ARG "2" CACHE STRING "Result from TRY_RUN" FORCE)
    set(OS_LINUX TRUE)

    set(WEBOS_APPINFO_ID "com.limelight.webos")
    set(CMAKE_INSTALL_RPATH "$ORIGIN/lib/backports:$ORIGIN/../lib/backports:$ORIGIN")
elseif (TARGET_STEAMLINK)
    set(OS_LINUX TRUE)
    set(CMAKE_INSTALL_PREFIX moonlight-tv)
    set(CMAKE_INSTALL_BINDIR moonlight-tv/bin)
    set(CMAKE_INSTALL_LIBDIR moonlight-tv/lib)
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

set(BUILD_SHARED_CORE_LIBS OFF)

if (TARGET_WEBOS)
    set(CMAKE_INSTALL_LIBDIR lib/backports)
    set(SDL2_BACKPORT_REVISION "webOS-2.30.x")
    include(ExternalSDL2BackportForWebOS)
    unset(CMAKE_INSTALL_LIBDIR)
else ()
    pkg_check_modules(SDL2 REQUIRED sdl2>=2.0.4)
endif()
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

find_package(MbedTLS)
if (NOT MBEDTLS_FOUND)
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    message(STATUS "Including mbedtls from source...")
    include(ExternalMbedTLS)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
endif ()

pkg_check_modules(OPUS opus)
if (NOT OPUS_FOUND)
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    message(STATUS "Including opus from source...")
    include(ExternalOPUS)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
endif ()

pkg_check_modules(INIH inih)
if (NOT INIH_FOUND)
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    message(STATUS "Including inih from source...")
    include(ExternalINIH)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
endif ()

if (TARGET_WEBOS)
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

if (TARGET_WEBOS)
    set(SS4S_MODULE_DISABLE_MMAL ON)
endif ()
option(SS4S_MODULE_BUILD_DUMMY "Enable SS4S dummy module" OFF)

add_subdirectory(third_party/ss4s)
add_subdirectory(third_party/lvgl EXCLUDE_FROM_ALL)
target_include_directories(lvgl PUBLIC ${CMAKE_SOURCE_DIR}/third_party/lvgl/src)
target_include_directories(lvgl PRIVATE ${CMAKE_SOURCE_DIR}/src/app/util)
target_include_directories(lvgl SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(lvgl SYSTEM PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(lvgl PUBLIC LV_CONF_PATH=../../../src/app/lvgl/lv_conf.h)
target_link_libraries(lvgl PUBLIC ${SDL2_LIBRARIES} ${FREETYPE_LIBRARIES})

add_subdirectory(third_party/commons)

target_include_directories(lv_gridview SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(lv_sdl_img SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

add_executable(moonlight src/main.c)
add_library(moonlight-lib STATIC)
target_link_libraries(moonlight PRIVATE moonlight-lib)

target_compile_lint(moonlight-lib)

set_target_properties(moonlight-lib PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED TRUE
        )

target_compile_definitions(moonlight-lib PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
string(LENGTH ${CMAKE_SOURCE_DIR} SOURCE_DIR_LENGTH)
target_compile_definitions(moonlight-lib PUBLIC SOURCE_DIR_LENGTH=${SOURCE_DIR_LENGTH})
target_compile_definitions(moonlight-lib PUBLIC APP_VERSION="${PROJECT_VERSION}" APPLOG_HOST=1)
target_compile_definitions(moonlight-lib PUBLIC LV_LVGL_H_INCLUDE_SIMPLE=1)

target_include_directories(moonlight-lib PUBLIC ${CMAKE_SOURCE_DIR}/core/libgamestream)

target_include_directories(moonlight-lib PUBLIC ${CMAKE_SOURCE_DIR}/utils/include)

if (MINGW)
    target_link_libraries(moonlight-lib PUBLIC libintl.a libiconv.a)
elseif (OS_DARWIN)
    target_link_libraries(moonlight-lib PUBLIC libintl.a)
endif ()

target_include_directories(moonlight-lib SYSTEM PUBLIC ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS}
        ${FREETYPE_INCLUDE_DIRS} ${INIH_INCLUDE_DIRS})
target_link_libraries(moonlight-lib PUBLIC
        m ${CMAKE_DL_LIBS} Threads::Threads ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES}
        moonlight-common-c gamestream lvgl ${INIH_LIBRARIES} ${CURL_LIBRARIES}
        Fontconfig::Fontconfig ${FREETYPE_LIBRARIES})

target_link_libraries(moonlight-lib PUBLIC lv_gridview lv_sdl_img)

target_include_directories(moonlight-lib SYSTEM PUBLIC ${OPUS_INCLUDE_DIRS})
target_link_libraries(moonlight-lib PUBLIC ${OPUS_LIBRARIES})

target_include_directories(moonlight-lib SYSTEM PUBLIC ${MBEDTLS_INCLUDE_DIRS})
target_link_libraries(moonlight-lib PUBLIC ${MBEDCRYPTO_LIBRARY} ${MBEDX509_LIBRARY})

if (TARGET_WEBOS)
    set(FEATURE_FORCE_FULLSCREEN ON)
    set(FEATURE_I18N_GETTEXT OFF)
    set(FEATURE_I18N_LANGUAGE_SETTINGS ON)
    set(FEATURE_CHECK_MODULE_OS_VERSION ON)
    set(FEATURE_INPUT_LIBCEC OFF)
    set(FEATURE_WINDOW_FULLSCREEN_DESKTOP OFF)
    set(FEATURE_INPUT_EVMOUSE ON)

    target_compile_definitions(moonlight-lib PUBLIC TARGET_WEBOS OS_LINUX _GNU_SOURCE APPID="${WEBOS_APPINFO_ID}")
    target_compile_definitions(moonlight-lib PUBLIC GAMECONTROLLERDB_PLATFORM="webOS" GAMECONTROLLERDB_PLATFORM_USE="Linux")

    target_include_directories(moonlight-lib SYSTEM PUBLIC ${PBNJSON_C_INCLUDE_DIRS} ${PMLOG_INCLUDE_DIRS}
            ${WEBOSI18N_INCLUDE_DIRS})
    target_link_libraries(moonlight-lib PUBLIC ${PBNJSON_C_LIBRARIES} ${PMLOG_LIBRARIES} ${WEBOSI18N_LIBRARIES})
    target_link_libraries(moonlight-lib PUBLIC commons-luna-sync)
else ()
    if (TARGET_STEAMLINK)
        set(FEATURE_FORCE_FULLSCREEN ON)
        set(FEATURE_WINDOW_FULLSCREEN_DESKTOP OFF)
        target_compile_definitions(moonlight-lib PUBLIC TARGET_STEAMLINK)
    endif ()
    if (OS_DARWIN)
        target_link_directories(moonlight-lib PUBLIC ${SDL2_LIBRARIES})
        target_compile_definitions(moonlight-lib PUBLIC OS_DARWIN)
        target_compile_definitions(moonlight-lib PUBLIC GAMECONTROLLERDB_PLATFORM="Mac OS X")
    elseif (OS_LINUX)
        set(FEATURE_EMBEDDED_SHELL ON)
        target_compile_definitions(moonlight-lib PUBLIC OS_LINUX _GNU_SOURCE)
        target_compile_definitions(moonlight-lib PUBLIC GAMECONTROLLERDB_PLATFORM="Linux")
    elseif (OS_WINDOWS)
        target_compile_definitions(moonlight-lib PUBLIC OS_WINDOWS)
        target_compile_definitions(moonlight-lib PUBLIC GAMECONTROLLERDB_PLATFORM="Windows")
    else ()
        message(FATAL_ERROR "Unknown build platform")
    endif ()

    set_target_properties(moonlight PROPERTIES OUTPUT_NAME moonlight-tv)
endif ()

if (FEATURE_INPUT_LIBCEC AND TARGET commons-cec-sdl)
    target_link_libraries(moonlight-lib PUBLIC commons-cec-sdl)
else ()
    set(FEATURE_INPUT_LIBCEC OFF)
endif ()

if (FEATURE_INPUT_EVMOUSE AND TARGET commons-evmouse)
    target_link_libraries(moonlight-lib PUBLIC commons-evmouse)
else ()
    set(FEATURE_INPUT_EVMOUSE OFF)
endif ()

target_link_libraries(moonlight-lib PUBLIC commons-ss4s-modules-list commons-sps-parser)

target_link_libraries(moonlight-lib PUBLIC commons-logging commons-gamecontrollerdb-updater commons-lazy
        commons-refcounter commons-executor commons-linked-list commons-wol commons-ini-writer commons-copyfile
        commons-sockaddr commons-hostport)

configure_file(src/app/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
target_include_directories(moonlight-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if (OS_WINDOWS)
    target_link_libraries(moonlight-lib PUBLIC rpcrt4)
else ()
    pkg_check_modules(UUID REQUIRED uuid)
    target_link_libraries(moonlight-lib PUBLIC ${UUID_LIBRARIES})
endif ()

if (WITH_SANITIZE)
    target_compile_options(moonlight-lib PUBLIC -fsanitize=${WITH_SANITIZE} -g3 -O0)
    target_link_options(moonlight-lib PUBLIC -fsanitize=${WITH_SANITIZE} -static-libasan -lrt)
endif ()

add_subdirectory(src)

if (NOT DEFINED CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR bin)
endif ()
install(TARGETS moonlight RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (BUILD_TESTS)
    add_subdirectory(third_party/Unity EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif ()

if (FEATURE_I18N_GETTEXT)
    include(MoonlightI18n)
endif ()

if (TARGET_WEBOS)
    include(PackageWebOS)
elseif (TARGET_STEAMLINK)
    include(PackageSteamLink)
elseif (OS_LINUX)
    include(PackageDebian)
elseif (OS_WINDOWS)
endif ()

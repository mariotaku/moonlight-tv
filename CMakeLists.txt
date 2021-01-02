cmake_minimum_required (VERSION 3.0)

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/utils/cmake)


include(WebOSPackage)

project(moonlight LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(Wayland REQUIRED IMPORTED_TARGET wayland-webos-client wayland-egl)
pkg_check_modules(OpenGL REQUIRED IMPORTED_TARGET egl glesv2)
pkg_check_modules(GLib REQUIRED IMPORTED_TARGET glib-2.0 gthread-2.0)
pkg_check_modules(GStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0)
pkg_check_modules(OpenSSL REQUIRED IMPORTED_TARGET openssl)
pkg_check_modules(cURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(UUID REQUIRED IMPORTED_TARGET uuid)
pkg_check_modules(Expat REQUIRED IMPORTED_TARGET expat)

add_subdirectory(system/libndl)
add_subdirectory(core/moonlight-common-c)
add_subdirectory(third_party/opus)

set(SOURCES_BASE
    app/main.c
    app/gst_sample.c
    app/ui/gui_root.c
    app/ui/computers_window.c
    app/ui/applications_window.c
    app/backend/computer_manager.c
    app/backend/computer_discovery.c
    app/backend/application_manager.c
    app/backend/error_manager.c
    core/moonlight-embedded/src/config.c
    core/moonlight-embedded/libgamestream/client.c
    core/moonlight-embedded/libgamestream/http.c
    core/moonlight-embedded/libgamestream/mkcert.c
    core/moonlight-embedded/libgamestream/xml.c
)

add_executable(moonlight ${SOURCES_BASE})

set_target_properties(moonlight PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED TRUE
)

target_compile_definitions(moonlight PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/core/moonlight-embedded)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/Nuklear)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/third_party/mdns)
target_include_directories(moonlight PRIVATE ${CMAKE_SOURCE_DIR}/utils/include)

target_link_libraries(moonlight PRIVATE
 PkgConfig::Wayland PkgConfig::OpenGL PkgConfig::GLib PkgConfig::GStreamer 
 PkgConfig::OpenSSL PkgConfig::cURL PkgConfig::UUID PkgConfig::Expat
 m pthread
 moonlight-common-c opus
 NDL_directmedia
)

set_target_properties(moonlight PROPERTIES
    WEBOS_APPINFO_ID com.limelight.webos
    WEBOS_APPINFO_VERSION 0.0.1
    WEBOS_APPINFO_VENDOR "Mariotaku Lee"
    WEBOS_APPINFO_ICON ${CMAKE_CURRENT_SOURCE_DIR}/deploy/webos/icon.png
    WEBOS_APPINFO_TITLE "Moonlight"
    WEBOS_APPINFO_EXTRA "nativeLifeCycleInterfaceVersion=2"
)

target_webos_package(moonlight)

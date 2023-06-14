set(PLATFORM_APP_SOURCE common/app_common.c)
set(PLATFORM_PATH_SOURCE sdl/path_sdl.c)
set(PLATFORM_I18N_SOURCE common/i18n_gettext.c)

if (TARGET_WEBOS)
    set(PLATFORM_APP_SOURCE webos/app_webos.c)
    set(PLATFORM_PATH_SOURCE webos/path_webos.c)
    set(PLATFORM_I18N_SOURCE webos/i18n_webos.c)
    add_subdirectory(webos)
elseif (OS_LINUX)
elseif (OS_WINDOWS)
elseif (OS_DARWIN)
else ()
    message(FATAL_ERROR "Unknown target OS")
endif ()

target_sources(moonlight PRIVATE
        sdl/bus.c
        sdl/input_manager.c
        sdl/navkey_sdl.c
        common/i18n_common.c
        ${PLATFORM_APP_SOURCE}
        ${PLATFORM_PATH_SOURCE}
        ${PLATFORM_I18N_SOURCE})
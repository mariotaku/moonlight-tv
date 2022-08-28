set(PLATFORM_APP_SOURCE common/app_common.c)
set(PLATFORM_PATH_SOURCE sdl/path_sdl.c)
set(PLATFORM_LOGGING_SOURCE common/logging_stdio.c)
set(PLATFORM_I18N_SOURCE common/i18n_gettext.c)

if (TARGET_WEBOS)
    set(PLATFORM_APP_SOURCE webos/app_webos.c)
    set(PLATFORM_PATH_SOURCE webos/path_webos.c)
    set(PLATFORM_LOGGING_SOURCE webos/logging_pmlog.c)
    set(PLATFORM_I18N_SOURCE webos/i18n_webos.c)
    set(PLATFORM_OSINFO_SOURCE webos/os_info.c)

    target_sources(moonlight PRIVATE
            webos/keyboard_webos.c
            webos/navkey_sdl_webos.c
            webos/os_info.c
            webos/pbnjson_compat.c
            webos/lunasynccall.c
            webos/SDL_webOS_compat.c
            )
elseif (OS_LINUX)
    set(PLATFORM_OSINFO_SOURCE linux/os_info_linux.c)
elseif (OS_WINDOWS)
    set(PLATFORM_OSINFO_SOURCE windows/os_info_windows.c)
else ()
    message(FATAL_ERROR "Unknown target OS")
endif ()


if (OS_LINUX)
    target_sources(moonlight PRIVATE linux/evmouse.c)
endif ()

target_sources(moonlight PRIVATE
        sdl/bus.c
        sdl/input_manager.c
        sdl/navkey_sdl.c
        common/i18n_common.c
        ${PLATFORM_APP_SOURCE}
        ${PLATFORM_PATH_SOURCE}
        ${PLATFORM_LOGGING_SOURCE}
        ${PLATFORM_I18N_SOURCE}
        ${PLATFORM_OSINFO_SOURCE})
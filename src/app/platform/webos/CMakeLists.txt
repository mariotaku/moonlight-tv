target_sources(moonlight-lib PRIVATE
        keyboard_webos.c
        navkey_sdl_webos.c
        pbnjson_compat.c
        SDL_webOS_compat.c
        app_wakelock_webos.c)

set(APP_WAKELOCK_IMPLEMENTED TRUE PARENT_SCOPE)
if (NOT APP_WAKELOCK_IMPLEMENTED)
    target_sources(moonlight-lib PRIVATE app_wakelock_common.c)
endif ()
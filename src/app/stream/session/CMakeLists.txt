target_sources(moonlight-lib PRIVATE
        session.c
        connection.c
        callbacks.c
        session_events.c
        session_worker.c
        session_priv.c)

if (FEATURE_INPUT_EVMOUSE)
    target_sources(moonlight-lib PRIVATE session_evmouse.c)
endif ()
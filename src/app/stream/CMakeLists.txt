target_sources(moonlight-lib PRIVATE session.c
        session_events.c
        session_worker.c
        session_priv.c
        embed_wrapper.c)

add_subdirectory(input)
add_subdirectory(connection)
add_subdirectory(audio)
add_subdirectory(video)

add_subdirectory(input)
add_subdirectory(session)
add_subdirectory(video)

target_sources(moonlight-lib PRIVATE settings.c)
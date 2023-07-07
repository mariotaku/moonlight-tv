add_subdirectory(input)
add_subdirectory(session)
add_subdirectory(video)
add_subdirectory(module)

target_sources(moonlight-lib PRIVATE settings.c)
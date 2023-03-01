add_subdirectory(input)
add_subdirectory(session)
add_subdirectory(video)
add_subdirectory(audio)
add_subdirectory(module)

target_sources(moonlight PRIVATE settings.c)
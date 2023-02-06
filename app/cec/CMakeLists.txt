pkg_check_modules(LIBCEC libcec)

if (LIBCEC_FOUND)
    set(IHSPLAY_FEATURE_LIBCEC ON PARENT_SCOPE)
    target_include_directories(moonlight SYSTEM PRIVATE ${LIBCEC_INCLUDE_DIRS})
    target_link_libraries(moonlight PRIVATE ${LIBCEC_LIBRARIES})

    target_sources(moonlight PRIVATE cec_support.c cec_key.c)
else ()
    message("libcec not found, disabling support")
    set(FEATURE_LIBCEC OFF PARENT_SCOPE)
endif ()
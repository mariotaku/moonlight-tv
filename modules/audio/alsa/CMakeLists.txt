find_package(ALSA)

if (ALSA_FOUND)
    add_library(moonlight-alsa SHARED alsa.c module.c)

    target_include_directories(moonlight-alsa PRIVATE ${ALSA_INCLUDE_DIRS})
    target_link_libraries(moonlight-alsa PRIVATE moonlight-module-api ${ALSA_LIBRARIES})
    if (TARGET opus)
        target_link_libraries(moonlight-alsa PRIVATE opus)
    else ()
        target_include_directories(moonlight-alsa PRIVATE ${OPUS_INCLUDE_DIRS})
        target_link_libraries(moonlight-alsa PRIVATE ${OPUS_LIBRARIES})
    endif ()
    target_compile_definitions(moonlight-alsa PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

    install(TARGETS moonlight-alsa LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()
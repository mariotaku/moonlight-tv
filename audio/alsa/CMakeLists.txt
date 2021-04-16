find_package(ALSA)

if (ALSA_FOUND)
    add_library(moonlight-alsa SHARED alsa.c module.c)

    target_include_directories(moonlight-alsa PRIVATE ${CMAKE_SOURCE_DIR}/app)

    target_include_directories(moonlight-alsa PRIVATE ${ALSA_INCLUDE_DIRS})
    target_link_libraries(moonlight-alsa PRIVATE moonlight-common-c opus ${ALSA_LIBRARIES})

    install(TARGETS moonlight-alsa LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
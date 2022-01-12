pkg_check_modules(PULSE REQUIRED libpulse-simple)

if (PULSE_FOUND)
    add_library(moonlight-pulse SHARED module.c pulse.c)

    target_include_directories(moonlight-pulse PRIVATE ${CMAKE_SOURCE_DIR}/app)

    target_include_directories(moonlight-pulse PRIVATE ${PULSE_INCLUDE_DIRS})
    target_link_libraries(moonlight-pulse PRIVATE moonlight-common-c-headers ${PULSE_LIBRARIES})
    if (TARGET opus)
        target_link_libraries(moonlight-pulse PRIVATE opus)
    else()
        target_include_directories(moonlight-pulse PRIVATE ${OPUS_INCLUDE_DIRS})
        target_link_libraries(moonlight-pulse PRIVATE ${OPUS_LIBRARIES})
    endif()
    target_compile_definitions(moonlight-pulse PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

    install(TARGETS moonlight-pulse LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
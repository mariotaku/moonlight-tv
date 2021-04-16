pkg_check_modules(PULSE REQUIRED libpulse-simple)

add_library(moonlight-pulse SHARED module.c pulse.c)

target_include_directories(moonlight-pulse PRIVATE ${CMAKE_SOURCE_DIR}/app)

target_include_directories(moonlight-pulse PRIVATE ${PULSE_INCLUDE_DIRS})
target_link_libraries(moonlight-pulse PRIVATE moonlight-common-c opus ${PULSE_LIBRARIES})

install(TARGETS moonlight-pulse LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
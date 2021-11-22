function(add_unit_test NAME SOURCES)
    add_executable(${NAME} ${SOURCES})
    target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/app)
    target_link_libraries(${NAME} unity)
    add_test(${NAME} ${NAME})
endfunction()

add_unit_test(sps_parser_tests app/stream/video/sps_parser_tests.c)
target_sources(sps_parser_tests PRIVATE ${CMAKE_SOURCE_DIR}/app/stream/video/sps_parser.c)

add_unit_test(sps_parser_tests sps_parser_tests.c)
target_sources(sps_parser_tests PRIVATE ${CMAKE_SOURCE_DIR}/app/stream/video/sps_parser.c)
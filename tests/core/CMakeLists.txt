add_unit_test(ml_plat_crypto_tests platform_crypto_tests.c)
target_link_libraries(ml_plat_crypto_tests PRIVATE moonlight-common-c Threads::Threads)
target_include_directories(ml_plat_crypto_tests PRIVATE ${CMAKE_SOURCE_DIR}/core/moonlight-common-c/src)
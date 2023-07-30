if (NOT BUILD_TESTS)
    return()
endif ()

find_package(OpenSSL 1.0 REQUIRED)

foreach (suite_name aes challenge hash sign)
    add_executable(test-gamestream-crypt-${suite_name} ${suite_name}.c)
    target_include_directories(test-gamestream-crypt-${suite_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libgamestream ${CMAKE_CURRENT_SOURCE_DIR}/../src)
    target_link_libraries(test-gamestream-crypt-${suite_name} PRIVATE ${OPENSSL_LIBRARIES})
    target_link_libraries(test-gamestream-crypt-${suite_name} PRIVATE ${MBEDCRYPTO_LIBRARY})
    target_include_directories(test-gamestream-crypt-${suite_name} SYSTEM PRIVATE ${MBEDTLS_INCLUDE_DIRS})
    add_test(test-gamestream-crypt-${suite_name} test-gamestream-crypt-${suite_name})
endforeach ()
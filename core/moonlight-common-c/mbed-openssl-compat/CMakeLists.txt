cmake_minimum_required(VERSION 3.1)

add_library(mbed-openssl-compat
 src/rand_impl.c
 src/evp_impl.c
)

target_include_directories(mbed-openssl-compat PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/openssl>"
)
target_link_libraries(mbed-openssl-compat PRIVATE mbedtls)


find_package(OpenSSL 1.0)

add_executable(test-evp src/evp_impl.c test/evp.c)
target_include_directories(test-evp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/openssl)
target_link_libraries(test-evp mbedtls ${OPENSSL_LIBRARIES})

add_test(test-mbed-openssl-compat test-evp)

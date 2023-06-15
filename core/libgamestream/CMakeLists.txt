cmake_minimum_required(VERSION 3.13)

if (NOT MBEDTLS_FOUND)
    message(FATAL_ERROR "MbedTLS not found!")
endif()

add_library(gamestream)
add_subdirectory(src)

set_target_properties(gamestream PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED TRUE)

if (OS_WINDOWS)
    target_link_libraries(gamestream PRIVATE rpcrt4)
else()
    pkg_check_modules(UUID REQUIRED uuid)
    target_link_libraries(gamestream PRIVATE ${UUID_LIBRARIES})
endif()

target_include_directories(gamestream PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libgamestream>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

target_link_libraries(gamestream PRIVATE moonlight-common-c)

target_link_libraries(gamestream PRIVATE ${MBEDCRYPTO_LIBRARY} ${MBEDX509_LIBRARY})
target_include_directories(gamestream SYSTEM PRIVATE ${MBEDTLS_INCLUDE_DIRS})
target_link_libraries(gamestream PRIVATE ${CURL_LIBRARIES} ${EXPAT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
if (BUILD_SHARED_LIBS)
    install(TARGETS gamestream LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (H264BITSTREAM_FOUND)
    add_library(gamestream-sps SHARED src/sps.c)

    target_include_directories(gamestream-sps PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libgamestream>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    )

    target_link_libraries(gamestream-sps PRIVATE h264bitstream moonlight-common-c)
    install(TARGETS gamestream-sps LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (BUILD_TESTING)
    find_package(OpenSSL 1.0 REQUIRED)

    foreach(suite_name aes challenge hash sign)
        add_executable(test-gamestream-crypt-${suite_name} test/${suite_name}.c)
        target_include_directories(test-gamestream-crypt-${suite_name} PUBLIC libgamestream src)
        target_link_libraries(test-gamestream-crypt-${suite_name} PRIVATE ${OPENSSL_LIBRARIES})
        target_link_libraries(test-gamestream-crypt-${suite_name} PRIVATE ${MBEDCRYPTO_LIBRARY})
        target_include_directories(test-gamestream-crypt-${suite_name} SYSTEM PRIVATE ${MBEDTLS_INCLUDE_DIRS})
        add_test(test-gamestream-crypt-${suite_name} test-gamestream-crypt-${suite_name})
    endforeach()
endif(BUILD_TESTING)
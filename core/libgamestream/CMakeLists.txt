cmake_minimum_required(VERSION 3.1)

option(GS_SPS "" OFF)

set(HEADERS 
    libgamestream/client.h
    libgamestream/http.h
    libgamestream/mkcert.h
    libgamestream/xml.h
)

set(SRC 
    src/client.c
    src/http.c
    src/mkcert.c
    src/xml.c
)

if (GS_SPS)
    list(APPEND HEADERS libgamestream/sps.h)
    list(APPEND SRC src/sps.c)
endif()

add_library(gamestream ${HEADERS} ${SRC})

set_target_properties(gamestream PROPERTIES
    PUBLIC_HEADER         libgamestream/client.h
    VERSION               ${PROJECT_VERSION}
    SOVERSION             ${PROJECT_VERSION_MAJOR}
)

target_include_directories(gamestream PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libgamestream>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)

target_link_libraries(gamestream moonlight-common-c)
target_link_libraries(gamestream mbedtls)
target_link_libraries(gamestream ${CURL_LIBRARIES} ${EXPAT_LIBRARIES} ${UUID_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
if (GS_SPS)
    target_link_libraries(gamestream h264bitstream)
endif()

find_package(OpenSSL 1.0)

foreach(suite_name aes challenge hash sign)
    add_executable(test-gamestream-crypt-${suite_name} test/${suite_name}.c)
    target_include_directories(test-gamestream-crypt-${suite_name} PUBLIC libgamestream src)
    target_link_libraries(test-gamestream-crypt-${suite_name} mbedtls ${OPENSSL_LIBRARIES})
    add_test(test-gamestream-crypt-${suite_name} test-gamestream-crypt-${suite_name})
endforeach()


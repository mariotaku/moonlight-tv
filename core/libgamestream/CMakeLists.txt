cmake_minimum_required(VERSION 3.1)
project(gamestream)

set(PROJECT_NAME "gamestream")
set(PROJECT_VERSION_MAJOR 4)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

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

add_library(gamestream ${HEADERS} ${SRC})

set_target_properties(gamestream PROPERTIES
    VERSION               ${PROJECT_VERSION}
    SOVERSION             ${PROJECT_VERSION_MAJOR}
)

target_include_directories(gamestream SYSTEM PRIVATE ${OPENSSL_INCLUDE_DIR})
target_include_directories(gamestream PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libgamestream>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

target_link_libraries(gamestream moonlight-common-c ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXPAT_LIBRARIES} ${UUID_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

install(TARGETS gamestream DESTINATION ${CMAKE_INSTALL_LIBDIR})

cmake_minimum_required(VERSION 3.1)
project(ilclient)

set(PROJECT_NAME "ilclient")
set(PROJECT_VERSION_MAJOR 4)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

set(BIN_INSTALL_DIR bin CACHE STRING "Binary installation directory relative to the install prefix")
set(LIB_INSTALL_DIR lib CACHE STRING "Library installation directory relative to the install prefix")
set(INCLUDE_INSTALL_DIR include CACHE STRING "Header installation directory relative to the install prefix")

set(HEADERS 
    ilclient.h
)

set(SRC 
    ilclient.c
    ilcore.c
)

find_package(Broadcom REQUIRED)

add_library(ilclient ${HEADERS} ${SRC})

set_target_properties(ilclient PROPERTIES
    PUBLIC_HEADER         ilclient.h
    VERSION               ${PROJECT_VERSION}
    SOVERSION             ${PROJECT_VERSION_MAJOR}
)

target_include_directories(ilclient PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)

install(TARGETS ilclient
    EXPORT        ilclient-export
    RUNTIME       DESTINATION "${BIN_INSTALL_DIR}"
    LIBRARY       DESTINATION "${LIB_INSTALL_DIR}"
    ARCHIVE       DESTINATION "${LIB_INSTALL_DIR}"
    PUBLIC_HEADER DESTINATION "${INCLUDE_INSTALL_DIR}"
)


target_include_directories(ilclient PRIVATE ${BROADCOM_INCLUDE_DIRS})
target_link_libraries(ilclient PRIVATE ${BROADCOM_OMX_LIBRARIES})
target_compile_definitions(ilclient PUBLIC ${BROADCOM_OMX_DEFINITIONS})
cmake_minimum_required(VERSION 3.1)
project(ilclient)

set(PROJECT_NAME "ilclient")
set(PROJECT_VERSION_MAJOR 4)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

set(HEADERS 
    ilclient.h
)

set(SRC 
    ilclient.c
    ilcore.c
)

find_package(Broadcom REQUIRED)

add_library(ilclient ${HEADERS} ${SRC})

target_include_directories(ilclient PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

target_include_directories(ilclient PRIVATE ${BROADCOM_INCLUDE_DIRS})
target_link_libraries(ilclient PRIVATE ${BROADCOM_OMX_LIBRARIES})
target_compile_definitions(ilclient PUBLIC ${BROADCOM_OMX_DEFINITIONS})

if (${BUILD_SHARED_LIBS})
    install(TARGETS ilclient LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
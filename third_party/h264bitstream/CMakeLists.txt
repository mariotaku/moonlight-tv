cmake_minimum_required(VERSION 3.1)
project(h264bitstream)

set(PROJECT_NAME "h264bitstream")
set(PROJECT_VERSION_MAJOR 4)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

set(HEADERS 
    bs.h
    h264_sei.h
    h264_stream.h
)

set(SRC 
    h264_nal.c
    h264_sei.c
    h264_stream.c
)

add_library(h264bitstream ${HEADERS} ${SRC})
set(H264BITSTREAM_FOUND 1 PARENT_SCOPE)

set_target_properties(h264bitstream PROPERTIES
    PUBLIC_HEADER         h264_stream.h
    VERSION               ${PROJECT_VERSION}
    SOVERSION             ${PROJECT_VERSION_MAJOR}
)

target_include_directories(h264bitstream PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)
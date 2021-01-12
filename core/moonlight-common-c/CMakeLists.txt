cmake_minimum_required(VERSION 3.1)
project(moonlight-common-c)

set(PROJECT_NAME "MoonlightCore")
set(PROJECT_DESCRIPTION "Moonlight Streaming Core Library")
set(PROJECT_AUTHOR "moonlight-stream")
set(PROJECT_URL "https://github.com/moonlight-stream/moonlight-common-c")

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

set(BIN_INSTALL_DIR bin CACHE STRING "Binary installation directory relative to the install prefix")
set(LIB_INSTALL_DIR lib CACHE STRING "Library installation directory relative to the install prefix")
set(INCLUDE_INSTALL_DIR include CACHE STRING "Header installation directory relative to the install prefix")

set(HEADERS
    "moonlight-common-c/enet/include/enet/callbacks.h"
    "moonlight-common-c/enet/include/enet/enet.h"
    "moonlight-common-c/enet/include/enet/list.h"
    "moonlight-common-c/enet/include/enet/protocol.h"
    "moonlight-common-c/enet/include/enet/time.h"
    "moonlight-common-c/enet/include/enet/types.h"
    "moonlight-common-c/enet/include/enet/unix.h"
    "moonlight-common-c/enet/include/enet/utility.h"
    "moonlight-common-c/enet/include/enet/win32.h"
    "moonlight-common-c/reedsolomon/rs.h"
    "moonlight-common-c/src/ByteBuffer.h"
    "moonlight-common-c/src/Input.h"
    "moonlight-common-c/src/Limelight-internal.h"
    "moonlight-common-c/src/Limelight.h"
    "moonlight-common-c/src/LinkedBlockingQueue.h"
    "moonlight-common-c/src/Platform.h"
    "moonlight-common-c/src/PlatformSockets.h"
    "moonlight-common-c/src/PlatformThreads.h"
    "moonlight-common-c/src/RtpFecQueue.h"
    "moonlight-common-c/src/RtpReorderQueue.h"
    "moonlight-common-c/src/Rtsp.h"
)

# Update with `find moonlight-common-c/ -name '*.c' -printf '    "%p"\n'`
set(SRC
    "moonlight-common-c/enet/callbacks.c"
    "moonlight-common-c/enet/compress.c"
    "moonlight-common-c/enet/host.c"
    "moonlight-common-c/enet/list.c"
    "moonlight-common-c/enet/packet.c"
    "moonlight-common-c/enet/peer.c"
    "moonlight-common-c/enet/protocol.c"
    "moonlight-common-c/enet/unix.c"
    "moonlight-common-c/enet/win32.c"
    "moonlight-common-c/reedsolomon/rs.c"
    "moonlight-common-c/src/AudioStream.c"
    "moonlight-common-c/src/ByteBuffer.c"
    "moonlight-common-c/src/Connection.c"
    "moonlight-common-c/src/ConnectionTester.c"
    "moonlight-common-c/src/ControlStream.c"
    "moonlight-common-c/src/FakeCallbacks.c"
    "moonlight-common-c/src/InputStream.c"
    "moonlight-common-c/src/LinkedBlockingQueue.c"
    "moonlight-common-c/src/Misc.c"
    "moonlight-common-c/src/Platform.c"
    "moonlight-common-c/src/PlatformSockets.c"
    "moonlight-common-c/src/RtpFecQueue.c"
    "moonlight-common-c/src/RtpReorderQueue.c"
    "moonlight-common-c/src/RtspConnection.c"
    "moonlight-common-c/src/RtspParser.c"
    "moonlight-common-c/src/SdpGenerator.c"
    "moonlight-common-c/src/SimpleStun.c"
    "moonlight-common-c/src/VideoDepacketizer.c"
    "moonlight-common-c/src/VideoStream.c"
)

add_library(moonlight-common-c ${INCLUDES} ${SRC})

set_target_properties(moonlight-common-c PROPERTIES
    PUBLIC_HEADER         moonlight-common-c/src/Limelight.h
    VERSION               ${PROJECT_VERSION}
    SOVERSION             ${PROJECT_VERSION_MAJOR}
)

target_compile_definitions(moonlight-common-c PRIVATE
    HAS_SOCKLEN_T=1
    HAS_FCNTL=1
    NO_MSGAPI=1
)

target_include_directories(moonlight-common-c PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/moonlight-common-c/enet/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/moonlight-common-c/reedsolomon>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/moonlight-common-c/src>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)

if (OPENSSL_FOUND)
    target_link_libraries(moonlight-common-c PRIVATE OpenSSL::Crypto)
else()
    target_link_libraries(moonlight-common-c PRIVATE PkgConfig::OpenSSL)
endif()

install(TARGETS moonlight-common-c
    EXPORT        moonlight-common-c-export
    RUNTIME       DESTINATION "${BIN_INSTALL_DIR}"
    LIBRARY       DESTINATION "${LIB_INSTALL_DIR}"
    ARCHIVE       DESTINATION "${LIB_INSTALL_DIR}"
    PUBLIC_HEADER DESTINATION "${INCLUDE_INSTALL_DIR}"
)

install(EXPORT moonlight-common-c-export
    FILE        moonlight-common-c-config.cmake
    DESTINATION "${LIB_INSTALL_DIR}/cmake/moonlight-common-c"
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/moonlight-common-c-config-version.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/moonlight-common-c-config-version.cmake"
    DESTINATION "${LIB_INSTALL_DIR}/cmake/moonlight-common-c"
)

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "${PROJECT_AUTHOR}")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "Documentation generated for the library")
set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "Examples")
set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "Sample applications using the library")

include(CPack)

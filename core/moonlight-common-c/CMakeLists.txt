cmake_minimum_required(VERSION 3.1)

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
    "moonlight-common-c/src/PlatformCrypto.c"
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

add_library(moonlight-common-c ${SRC})

set_target_properties(moonlight-common-c PROPERTIES
    PUBLIC_HEADER         moonlight-common-c/src/Limelight.h
    VERSION               ${PROJECT_VERSION}
    SOVERSION             ${PROJECT_VERSION_MAJOR}
)

target_compile_definitions(moonlight-common-c PRIVATE
    HAS_SOCKLEN_T=1
    HAS_FCNTL=1
    NO_MSGAPI=1
    USE_MBEDTLS=1
)

target_include_directories(moonlight-common-c PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/moonlight-common-c/enet/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/moonlight-common-c/reedsolomon>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/moonlight-common-c/src>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mbed-openssl-compat>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)

target_link_libraries(moonlight-common-c mbedcrypto)
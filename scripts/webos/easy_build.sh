#!/usr/bin/env bash

if [ ! -d app ] || [ ! -f CMakeLists.txt ]; then
  echo "Please invoke this script in project root directory"
  exit 1
fi

CMAKE_BIN=$(which cmake)
CPACK_BIN=$(which cpack)

if [ ! -x "$CMAKE_BIN" ]; then
  echo "Please install CMake."
  exit 1
fi

if [ -z "${CMAKE_BINARY_DIR}" ]; then
  CMAKE_BINARY_DIR=build/webos-easy_build
fi

if [ -z "$CI" ]; then
  echo "Update submodules"
  git submodule update --init --recursive
fi

if [ -z "${TOOLCHAIN_FILE}" ]; then
  echo "Setup environment"
  . /opt/webos-sdk-x86_64/1.0.g/environment-setup-armv7a-neon-webos-linux-gnueabi
  TOOLCHAIN_FILE=/opt/webos-sdk-x86_64/1.0.g/sysroots/x86_64-webossdk-linux/usr/share/cmake/OEToolchainConfig.cmake
fi

echo "Configure project"
if [ ! -d "${CMAKE_BINARY_DIR}" ]; then
  mkdir -p "${CMAKE_BINARY_DIR}"
fi

BUILD_OPTIONS="-DTARGET_WEBOS=ON -DBUILD_TESTS=OFF"

# shellcheck disable=SC2068,SC2086
$CMAKE_BIN -B"${CMAKE_BINARY_DIR}" -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" $BUILD_OPTIONS $@ || exit 1

$CMAKE_BIN --build "${CMAKE_BINARY_DIR}" -- -j "$(nproc)" || exit 1

echo "Build package"
cd "${CMAKE_BINARY_DIR}" || exit 1
$CPACK_BIN
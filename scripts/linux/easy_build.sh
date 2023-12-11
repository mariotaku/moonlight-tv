#!/bin/bash

#Get kernel release name
os=$(uname -r)

if [ ! -f scripts/linux/easy_build.sh ]; then
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
  CMAKE_BINARY_DIR=build/linux-easy_build
fi

if [ -z "$CI" ]; then
  echo "Update submodules"
  git submodule update --init --recursive
fi

echo "Install dependencies. You may be prompted to enter password"
case $os in
  *arch*|*Arch*)
	echo Detected system as Arch base running kernel $os;
	sudo pacman -S openssl cmake ffmpeg sdl2_image sdl2 opus curl mbedtls fontconfig expat libinih
    ;;
  *)
	echo Detected system as Debian base running kernel $os;
	sudo apt-get install -y build-essential cmake
	sudo apt-get install -y libsdl2-dev libsdl2-image-dev libopus-dev uuid-dev    \
	     libcurl4-openssl-dev libavcodec-dev libavutil-dev libexpat1-dev          \
	     libmbedtls-dev libfontconfig1-dev libinih-dev
    ;;
esac

echo "Configure project"
if [ ! -d "${CMAKE_BINARY_DIR}" ]; then
  mkdir -p "${CMAKE_BINARY_DIR}"
fi

BUILD_OPTIONS="-DBUILD_TESTS=OFF"

# shellcheck disable=SC2068,SC2086
$CMAKE_BIN -B"${CMAKE_BINARY_DIR}" -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" $BUILD_OPTIONS $@ || exit 1

$CMAKE_BIN --build "${CMAKE_BINARY_DIR}" -- -j "$(nproc)" || exit 1

echo "Build package"
cd "${CMAKE_BINARY_DIR}" || exit 1
$CPACK_BIN

#!/bin/bash

TOPDIR=$(git rev-parse --show-toplevel)

if [ $TOPDIR != $PWD ]; then
    echo "Please invoke this script in project root directory"
    exit 1
fi

CMAKE_BIN=$(which cmake)

if [ -z $CMAKE_BIN ]; then
    echo "Please install CMake."
    exit 1
fi

echo "Update submodules"
git submodule update --init --recursive

echo "Setup environment"
. /opt/webos-sdk-x86_64/1.0.g/environment-setup-armv7a-neon-webos-linux-gnueabi

echo "Project configuration"
if [ ! -d build ]; then
    mkdir build
fi

cd build
$CMAKE_BIN .. -DCMAKE_TOOLCHAIN_FILE=/opt/webos-sdk-x86_64/1.0.g/sysroots/x86_64-webossdk-linux/usr/share/cmake/OEToolchainConfig.cmake -DTARGET_WEBOS=ON "$@"

echo "Start build"
$CMAKE_BIN --install .
$CMAKE_BIN --build . --target webos-package-moonlight "$@"
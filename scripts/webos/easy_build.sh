#!/bin/sh

TOPDIR=$(git rev-parse --show-toplevel)

if [ $TOPDIR != $PWD ]; then
    echo "Please invoke this script in project root directory"
    return 1
fi

echo "Update submodules"
git submodule update --init --recursive

echo "Project configuration"
if [ ! -d build ]; then
    mkdir build
fi

cd build
cmake .. -DTARGET_WEBOS=ON -DCMAKE_TOOLCHAIN_FILE=/opt/webos-sdk-x86_64/1.0.g/sysroots/x86_64-webossdk-linux/usr/share/cmake/OEToolchainConfig.cmake

echo "Start build"
make webos-package-moonlight
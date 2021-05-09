#!/bin/bash

TOPDIR=$(git rev-parse --show-toplevel)

if [ $TOPDIR != $PWD ]; then
    echo "Please invoke this script in project root directory"
    return 1
fi

echo "Update submodules"
git submodule update --init --recursive

echo "Install dependencies. You may be prompted to enter password"
sudo apt-get install build-essential cmake
sudo apt-get install libsdl2-dev libsdl2-image-dev libopus-dev libcurl4-openssl-dev uuid-dev\
    libavcodec-dev libavutil-dev libexpat1-dev libmbedtls-dev libraspberrypi-dev

echo "Project configuration"
if [ ! -d build ]; then
    mkdir build
fi

cd build
cmake .. -DTARGET_RASPI=ON -DCMAKE_BUILD_TYPE=Debug

echo "Start build"
cmake --build . --target moonlight -j $(nproc)

echo "Build package"
cpack
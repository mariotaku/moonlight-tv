#!/bin/sh

TARGET="$1"
cmake --build . --target "steamlink-install-${TARGET}"

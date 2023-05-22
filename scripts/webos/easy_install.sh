#!/bin/sh

TARGET="$1"
ARES_DEVICE=$(ares-device -d | xargs) cmake --build . --target "webos-launch-${TARGET}"

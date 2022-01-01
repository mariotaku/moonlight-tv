#!/bin/sh

export MOONLIGHT_OUTPUT_NOREDIR=1
export HOME=/media/developer/apps/usr/palm/applications/com.limelight.webos
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/lib
export PATH="$HOME/bin:$PATH"
export XDG_RUNTIME_DIR=/tmp/xdg
export APPID=com.limelight.webos

moonlight

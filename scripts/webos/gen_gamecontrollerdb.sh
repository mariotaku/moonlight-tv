#!/bin/bash

if [ ! -d assets ]; then
    mkdir assets
fi

echo "# Generated from https://github.com/gabomdq/SDL_GameControllerDB" > assets/gamecontrollerdb.txt
git -C third_party/SDL_GameControllerDB log -1 --pretty=format:"# %h %ad - %s%n" >> assets/gamecontrollerdb.txt
echo >> assets/gamecontrollerdb.txt
grep ',platform:Linux,$' third_party/SDL_GameControllerDB/gamecontrollerdb.txt | sed 's/,platform:Linux,$/,platform:webOS,/' >> assets/gamecontrollerdb.txt
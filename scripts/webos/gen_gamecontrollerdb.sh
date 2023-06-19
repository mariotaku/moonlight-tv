#!/bin/sh

echo "# Generated from https://github.com/gabomdq/SDL_GameControllerDB"
echo "# Generated at $(date)"
echo
curl -sL 'https://github.com/gabomdq/SDL_GameControllerDB/raw/master/gamecontrollerdb.txt' |
  sed -n 's/^\(.*\),platform:Linux,$/\1,platform:webOS,/p'

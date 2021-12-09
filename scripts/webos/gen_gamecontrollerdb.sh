#!/bin/sh

OUTDIR=deploy/webos/assets

if [ ! -d $OUTDIR ]; then
    mkdir -p $OUTDIR
fi

echo "# Generated from https://github.com/gabomdq/SDL_GameControllerDB" > $OUTDIR/gamecontrollerdb.txt
echo "# Generated at $(date)" >> $OUTDIR/gamecontrollerdb.txt
echo >> $OUTDIR/gamecontrollerdb.txt
curl -L 'https://github.com/gabomdq/SDL_GameControllerDB/raw/master/gamecontrollerdb.txt' \
 | grep ',platform:Linux,$'\
 | sed 's/,platform:Linux,$/,platform:webOS,/' >> $OUTDIR/gamecontrollerdb.txt
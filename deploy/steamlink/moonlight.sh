#!/bin/sh

# The default HOME is not persistent, so override
# it to a path on the onboard flash. Otherwise our
# pairing data will be lost each reboot.
HOME=/usr/local/moonlight
APPDIR=$(realpath "$(dirname "$0")")
LD_LIBRARY_PATH="$APPDIR/lib:$LD_LIBRARY_PATH"

# Renice PE_Single_CPU which seems to host A/V stuff
renice -10 -p $(pidof PE_Single_CPU)

# Renice Moonlight itself to avoid preemption by background tasks
# Write output to a logfile in /tmp
if [ -t 1 ]; then
  exec nice -n -10 ./bin/moonlight-tv
else
  exec nice -n -10 ./bin/moonlight-tv >/tmp/moonlight-tv.log 2>&1
fi

#!/bin/bash

# Usage:
# `fix-logs.sh ~/shower.log` to just count how many sequences would be dropped
# `fix-logs.sh ~/shower.log ~/shower.fix.log` to write filtered $1 to $2

# This script removes start/flow{n}/stop sequences with only n flow messages.
# The light switch made the flow interrupt on the Wemos D1 mini trigger sometimes. It's not the Hall effect sensor but some ripple issue since this does not happen when powered from the laptop instead of the wall power supply. Now if there is only one flow, messages are not sent anymore.

# sudo apt install pcregrep
inp="$1"
count() {
  echo "start/flow{$1}/stop sequences:"
  pcregrep -M 'start .*\n(.*flow .*\n){'$1'}.*stop' "$inp" | grep stop | wc -l
}
count 1 # 3156
count 2 # 147
count 3 # 3
count 4 # 0
echo "Will ignore {1,3}. Adjust if needed!"

if [ $# -eq 2 ]; then
  pcregrep -v -M 'start .*\n(.*flow .*\n){1,3}.*stop' "$1" > "$2"
  echo "Filtered $1 written to $2."
fi

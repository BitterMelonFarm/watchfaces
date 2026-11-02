#!/usr/bin/env bash
# Capture the store screenshots (exactly 260x260, no frame) from the Round 2 emulator.
# Run on your own machine with the Pebble SDK installed:  bash release/capture_screenshots.sh
# Uses GNU date (on macOS: brew install coreutils and use gdate). Output: screenshots/gabbro_*.png
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p screenshots

pebble build
pebble install --emulator gabbro

# shot NAME "DATE EXPRESSION" STEPS BATTERY%
shot() {
  local epoch
  epoch=$(date -d "$2" +%s)
  pebble emu-set-time --emulator gabbro "$epoch"
  pebble emu-steps --emulator gabbro "$3"
  pebble emu-battery --emulator gabbro --percent "$4"
  sleep 4                                   # let the face redraw (it ticks once a second)
  pebble screenshot --emulator gabbro --no-open "screenshots/gabbro_$1.png"
  echo "captured screenshots/gabbro_$1.png"
}

shot code   "next tuesday 10:09"   3200 80   # weekday: coding at the desk
shot play   "next tuesday 19:00"   8600 70   # goal reached: happy pet, kids and ball
shot story  "next tuesday 20:10"   6500 60   # bedtime story
shot walk   "next saturday 10:00"  2500 90   # weekend park walk
shot sleep  "next tuesday 23:30"   5000 50   # asleep

echo "Done. Upload the PNGs in screenshots/ as the gabbro screenshots (max 5)."

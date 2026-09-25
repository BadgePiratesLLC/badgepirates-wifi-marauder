#!/usr/bin/env bash
# BSidesKC Badge simulator — Nexus 0f35e128. Builds the native "badge_sim"
# host target: real UI code (badge_menu.cpp/touch_input.cpp/badge_nav.cpp)
# against fakes of the hardware seam. See sim/README.md.
set -euo pipefail
cd "$(dirname "$0")/.."   # repo root

OUT=sim/out
mkdir -p "$OUT"

clang++ -std=c++17 -O1 -g \
  -DSIM_BUILD \
  -Wno-c++11-narrowing \
  -Isim/fakes -Isim/fakes/hardware -Isrc -Iinclude \
  sim/src/sim_main.cpp \
  sim/src/hw_stubs.cpp \
  sim/fakes/Display.cpp \
  sim/fakes/MenuFunctions.cpp \
  src/hardware/badge_menu.cpp \
  src/hardware/touch_input.cpp \
  src/hardware/badge_nav.cpp \
  -lz \
  -o "$OUT/badge_sim"

echo "built $OUT/badge_sim"

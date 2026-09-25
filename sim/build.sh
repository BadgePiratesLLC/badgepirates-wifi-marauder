#!/usr/bin/env bash
# BSidesKC Badge simulator — Nexus 0f35e128, LVGL port Nexus 84f4e52c.
# Builds the native "badge_sim" host target: real UI code
# (badge_menu.cpp/touch_input.cpp/badge_nav.cpp/UI/CardKit.cpp) against
# fakes of the hardware seam, and a real LVGL 9.2.2 (vendored as a
# submodule at sim/vendor/lvgl, pinned to the same commit as CC15's
# QACode_27/sim/vendor/lvgl) doing the actual compositing. See sim/README.md.
set -euo pipefail
cd "$(dirname "$0")/.."   # repo root

OUT=sim/out
OBJDIR=sim/.objs/lvgl
mkdir -p "$OUT" "$OBJDIR"

LVGL_DIR=sim/vendor/lvgl
COMMON_FLAGS=(-O1 -g -DSIM_BUILD -DLV_CONF_INCLUDE_SIMPLE -Wno-c++11-narrowing \
  -Isim/fakes -Isim/fakes/hardware -Isrc -Iinclude -I"$LVGL_DIR")

# LVGL's own .c sources, object-cached by content hash of the file list so
# a rebuild after touching only badge_menu.cpp/CardKit.cpp doesn't
# recompile all of LVGL every time.
LVGL_SRCS=$(find "$LVGL_DIR/src" -name '*.c')
LVGL_OBJS=()
for f in $LVGL_SRCS; do
  obj="$OBJDIR/$(echo "$f" | tr '/' '_').o"
  if [ ! -f "$obj" ] || [ "$f" -nt "$obj" ]; then
    cc -std=c11 "${COMMON_FLAGS[@]}" -c -o "$obj" "$f"
  fi
  LVGL_OBJS+=("$obj")
done

clang++ -std=c++17 "${COMMON_FLAGS[@]}" \
  sim/src/sim_main.cpp \
  sim/src/hw_stubs.cpp \
  sim/fakes/Display.cpp \
  sim/fakes/MenuFunctions.cpp \
  sim/fakes/lv_disp_port_sim.cpp \
  src/hardware/badge_menu.cpp \
  src/hardware/touch_input.cpp \
  src/hardware/badge_nav.cpp \
  src/UI/CardKit.cpp \
  "${LVGL_OBJS[@]}" \
  -lz \
  -o "$OUT/badge_sim"

echo "built $OUT/badge_sim"

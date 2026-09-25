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

# Same BP_GIT_SHA build-time injection as scripts/inject_build_info.py does
# for the real firmware (Nexus 176cc276) - the sim's splash render has to
# show a real SHA too, not a placeholder, or it's not proving what the
# splash actually looks like.
BP_GIT_SHA=$(git rev-parse --short=8 HEAD 2>/dev/null || echo unknown)
if [ -n "$(git status --porcelain 2>/dev/null)" ]; then BP_GIT_SHA="${BP_GIT_SHA}-dirty"; fi

# Same upstream-configs.h extraction as scripts/inject_build_info.py does for
# the real firmware (Nexus 176cc276 QA fail #4) - the sim must read the
# actual upstream MARAUDER_VERSION, not a hand-copied literal, or the sim
# render stops proving what it claims to prove.
UPSTREAM_CONFIGS_H=esp32marauder-upstream/esp32_marauder/configs.h
BP_MARAUDER_VERSION_UPSTREAM=$(grep -oE '#define[[:space:]]+MARAUDER_VERSION[[:space:]]+"[^"]+"' "$UPSTREAM_CONFIGS_H" | grep -oE '"[^"]+"' | tr -d '"')
if [ -z "$BP_MARAUDER_VERSION_UPSTREAM" ]; then
  echo "sim/build.sh: could not find MARAUDER_VERSION in $UPSTREAM_CONFIGS_H" >&2
  exit 1
fi

COMMON_FLAGS=(-O1 -g -DSIM_BUILD -DLV_CONF_INCLUDE_SIMPLE -DBP_GIT_SHA="\"$BP_GIT_SHA\"" \
  -DBP_MARAUDER_VERSION_UPSTREAM="\"$BP_MARAUDER_VERSION_UPSTREAM\"" -Wno-c++11-narrowing \
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
  src/hardware/splash_screen.cpp \
  src/UI/CardKit.cpp \
  src/UI/StatusBar.cpp \
  src/UI/img_bp_skull.c \
  "${LVGL_OBJS[@]}" \
  -lz \
  -o "$OUT/badge_sim"

echo "built $OUT/badge_sim"

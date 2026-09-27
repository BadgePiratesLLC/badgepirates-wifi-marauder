#!/usr/bin/env bash
# BSidesKC Badge simulator — interactive WASM build (Nexus 7b1f65b2).
#
# Same source list as sim/build.sh (real badge_menu.cpp/touch_input.cpp/
# badge_nav.cpp/UI/CardKit.cpp against LVGL 9.2 in sim/vendor/lvgl and the
# sim/fakes/ hardware seam) but swaps sim/src/sim_main.cpp + sim_png.cpp for
# sim/src/sim_wasm_main.cpp, and em++ (Emscripten) for clang++. Nothing in
# src/, include/, or sim/fakes/ changes for this target — see
# sim/src/sim_wasm_main.cpp's header comment for why.
#
# Needs the Emscripten SDK on PATH (em++, emcc). Not vendored into this
# repo: `git clone https://github.com/emscripten-core/emsdk && cd emsdk &&
# ./emsdk install latest && ./emsdk activate latest && source
# emsdk_env.sh` once, anywhere outside the repo, then re-source
# emsdk_env.sh in any shell that runs this script.
set -euo pipefail
cd "$(dirname "$0")/.."   # repo root

if ! command -v em++ >/dev/null 2>&1; then
  echo "sim/build_wasm.sh: em++ not on PATH. Install/activate the Emscripten SDK first:" >&2
  echo "  git clone https://github.com/emscripten-core/emsdk ~/.toolchains/emsdk" >&2
  echo "  ~/.toolchains/emsdk/emsdk install latest && ~/.toolchains/emsdk/emsdk activate latest" >&2
  echo "  source ~/.toolchains/emsdk/emsdk_env.sh" >&2
  exit 1
fi

OUT=sim/out
OBJDIR=sim/.objs/lvgl-wasm
mkdir -p "$OUT" "$OBJDIR"

LVGL_DIR=sim/vendor/lvgl

# Same BP_GIT_SHA / MARAUDER_VERSION injection as sim/build.sh - see that
# script's comments for why this isn't a placeholder.
BP_GIT_SHA=$(git rev-parse --short=8 HEAD 2>/dev/null || echo unknown)
if [ -n "$(git status --porcelain 2>/dev/null)" ]; then BP_GIT_SHA="${BP_GIT_SHA}-dirty"; fi

UPSTREAM_CONFIGS_H=esp32marauder-upstream/esp32_marauder/configs.h
BP_MARAUDER_VERSION_UPSTREAM=$(grep -oE '#define[[:space:]]+MARAUDER_VERSION[[:space:]]+"[^"]+"' "$UPSTREAM_CONFIGS_H" | grep -oE '"[^"]+"' | tr -d '"')
if [ -z "$BP_MARAUDER_VERSION_UPSTREAM" ]; then
  echo "sim/build_wasm.sh: could not find MARAUDER_VERSION in $UPSTREAM_CONFIGS_H" >&2
  exit 1
fi

COMMON_FLAGS=(-O1 -g -DSIM_BUILD -DLV_CONF_INCLUDE_SIMPLE -DBP_GIT_SHA="\"$BP_GIT_SHA\"" \
  -DBP_MARAUDER_VERSION_UPSTREAM="\"$BP_MARAUDER_VERSION_UPSTREAM\"" -Wno-c++11-narrowing \
  -Isim/fakes -Isim/fakes/hardware -Isrc -Iinclude -I"$LVGL_DIR")

LVGL_SRCS=$(find "$LVGL_DIR/src" -name '*.c')
LVGL_OBJS=()
for f in $LVGL_SRCS; do
  obj="$OBJDIR/$(echo "$f" | tr '/' '_').o"
  if [ ! -f "$obj" ] || [ "$f" -nt "$obj" ]; then
    emcc -std=c11 "${COMMON_FLAGS[@]}" -c -o "$obj" "$f"
  fi
  LVGL_OBJS+=("$obj")
done

em++ -std=c++17 "${COMMON_FLAGS[@]}" \
  -sEXPORTED_FUNCTIONS=_main,_sim_wasm_pointer,_sim_wasm_screen_w,_sim_wasm_screen_h \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
  -sALLOW_MEMORY_GROWTH=1 \
  -sMODULARIZE=1 -sEXPORT_NAME=BadgeSimModule \
  -sENVIRONMENT=web \
  -sASYNCIFY=1 \
  sim/src/sim_wasm_main.cpp \
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
  -o "$OUT/badge_sim_wasm.js"

cp sim/wasm/index.html "$OUT/index.html"

echo "built $OUT/badge_sim_wasm.js + .wasm, page at $OUT/index.html"
echo "serve it (wasm needs an http origin, not file://):"
echo "  cd $OUT && python3 -m http.server 8765"

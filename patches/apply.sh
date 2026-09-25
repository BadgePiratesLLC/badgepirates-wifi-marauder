#!/usr/bin/env bash
# Applies our small, necessary patches to the esp32marauder-upstream
# submodule. We don't have push access to justcallmekoko/ESP32Marauder
# (it's the real upstream, not a fork we control), so these can't be
# committed into the submodule itself - `git submodule update` would
# just reset them away. Run this once after `git submodule update --init`
# and again any time the submodule pointer moves.
#
# Without this, boards that don't define HAS_GPS (every BSidesKC badge
# today - see include/marauder_config.h) fail to compile: RunSetup()
# unconditionally touches gpsPOIMenu, which only exists in the class
# under #ifdef HAS_GPS.
set -euo pipefail
cd "$(dirname "$0")/../esp32marauder-upstream"
for p in ../patches/esp32marauder-upstream-*.patch; do
  if git apply --reverse --check "$p" 2>/dev/null; then
    echo "already applied: $(basename "$p")"
  else
    echo "applying: $(basename "$p")"
    git apply "$p"
  fi
done

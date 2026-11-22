#!/bin/bash
# Build and run the desktop UI preview. Output PNGs go to tools/ui_preview/out/.
# Needs a C++ compiler and FreeType (default: the conda env in ~/ps4-tools/extra).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
APP="$HERE/../../app"
FT_PREFIX="${FT_PREFIX:-$HOME/ps4-tools/extra}"
OUT="$HERE/out"
mkdir -p "$OUT"
rm -rf "$OUT/data" && cp -R "$HERE/sample/data" "$OUT/data"
clang++ -std=c++17 -O1 -g -w -DGRAPHICS_USES_FONT -DAPP0="\"$APP\"" \
    -I"$HERE/stubs" -I"$APP/src" -I"$APP/src/common" -I"$HERE/../../include" \
    -I"$FT_PREFIX/include/freetype2" \
    "$HERE/preview_main.cpp" "$HERE/host_platform.cpp" \
    "$APP/src/ui.cpp" "$APP/src/common/graphics.cpp" "$APP/src/ps_background.cpp" \
    "$APP/src/hid_profile.cpp" "$APP/src/config_manager.cpp" \
    "$APP/src/ui_features.cpp" "$APP/src/companion_features.cpp" "$APP/src/i18n.cpp" \
    -L"$FT_PREFIX/lib" -lfreetype -Wl,-rpath,"$FT_PREFIX/lib" -o "$OUT/preview"
PREVIEW_DATA_DIR="$OUT" "$OUT/preview" "$OUT"

#!/bin/bash
# Desktop tests (no PS4 needed): Switch Pro Bluetooth decoder/kernel-patch logic and Companion App file formats.
# Kernel (ring 0) code is compiled but never executed: the kexec syscall is intercepted.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$HERE/.."
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
SAN=(-g -O1 -fsanitize=address,undefined -w)

echo "== Switch Pro Bluetooth (daemon/src/bt_driver.c)"
clang -target x86_64-apple-macos13 "${SAN[@]}" -isystem "$HERE/daemon_stubs" -I"$ROOT/daemon/include" -I"$ROOT/include" \
    -I"$ROOT/daemon/src" -D__PS4__ -D__ORBIS__ -o "$OUT/test_switch_bt" "$HERE/test_switch_bt.c"
"$OUT/test_switch_bt" | tail -1

echo "== Companion App features (app/src/companion_features.cpp)"
clang++ -std=c++17 "${SAN[@]}" -I"$ROOT/tools/ui_preview/stubs" -I"$ROOT/app/src" -I"$ROOT/app/src/common" -I"$ROOT/include" \
    -I"${FT_PREFIX:-$HOME/ps4-tools/extra}/include/freetype2" -DGRAPHICS_USES_FONT \
    "$HERE/test_companion_features.cpp" "$ROOT/app/src/companion_features.cpp" "$ROOT/tools/ui_preview/host_platform.cpp" \
    -o "$OUT/test_features"
PREVIEW_DATA_DIR="$OUT/data" "$OUT/test_features" | tail -1

#!/bin/bash
# macOS port of build_daemon.ps1 + build_app.ps1 (same flags, same steps).
# Usage: ./build_mac.sh            -> bin/omnipad.bin + bin/omnipad_companion.pkg
#        ./build_mac.sh daemon     -> bin/omnipad.bin only
set -euo pipefail

TOOLCHAIN="${OO_PS4_TOOLCHAIN:-$HOME/ps4-tools/openorbis-v0.5.4/OpenOrbis/PS4Toolchain}"
export OO_PS4_TOOLCHAIN="$TOOLCHAIN"   # required by create-fself
LLVM_BIN="${LLVM_BIN:-$HOME/ps4-tools/llvm-21/bin}"   # clang/lld 21.1.8, same as the official build
PROJECT="$(cd "$(dirname "$0")" && pwd)"
DAEMON_DIR="$PROJECT/daemon"
APP_DIR="$PROJECT/app"
CLANG="$LLVM_BIN/clang"
CLANGXX="$LLVM_BIN/clang++-21"
LLD="$LLVM_BIN/ld.lld"
CREATE_FSELF="$TOOLCHAIN/bin/macos/create-fself-macos"
PKGTOOL="$TOOLCHAIN/bin/macos/PkgTool.Core"
CONTENT_ID="UP9000-OMNI00001_00-OMNIPADCOMPANION"

mkdir -p "$DAEMON_DIR/obj" "$PROJECT/bin"

echo "=== Synchronizing Version Header ==="
python3 -I "$PROJECT/scripts/generate_version.py"

echo "=== Compiling OmniPad Daemon ==="
CFLAGS=(--target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -fno-delete-null-pointer-checks -c -Wall
        -isysroot "$TOOLCHAIN" -isystem "$TOOLCHAIN/include" -isystem "$TOOLCHAIN/include/orbis/_types"
        -I"$DAEMON_DIR/include" -I"$PROJECT/include" -D__PS4__ -D__ORBIS__)
DAEMON_SRCS=(daemon_syscalls main vpad usb_driver config log controller_detect translator generic_hid bt_driver)
DAEMON_OBJS=()
for n in "${DAEMON_SRCS[@]}"; do
    echo "  $n.c"
    "$CLANG" "${CFLAGS[@]}" -o "$DAEMON_DIR/obj/$n.o" "$DAEMON_DIR/src/$n.c"
    DAEMON_OBJS+=("$DAEMON_DIR/obj/$n.o")
done
"$LLD" -m elf_x86_64 -pie --script "$TOOLCHAIN/link.x" --eh-frame-hdr -L"$TOOLCHAIN/lib" \
    -o "$PROJECT/bin/omnipad.elf" "${DAEMON_OBJS[@]}" -lc -lkernel
cp -f "$PROJECT/bin/omnipad.elf" "$PROJECT/bin/omnipad.bin"
echo "=== Daemon OK: bin/omnipad.bin ==="

[ "${1:-}" = "daemon" ] && exit 0

echo "=== Building Companion App ==="
mkdir -p "$APP_DIR/obj" "$APP_DIR/bin" "$APP_DIR/assets/payload"
cp -f "$PROJECT/bin/omnipad.bin" "$APP_DIR/assets/payload/omnipad.bin"
python3 -I "$APP_DIR/make_sfo_app.py"

CXXFLAGS=(--target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -fno-exceptions -fno-rtti -c -Wall -O2
          -DGRAPHICS_USES_FONT -isysroot "$TOOLCHAIN" -isystem "$TOOLCHAIN/include" -isystem "$TOOLCHAIN/include/c++/v1"
          -I"$APP_DIR/src" -I"$APP_DIR/src/common" -I"$PROJECT/include")
APP_SRCS=(main config_manager installer hid_profile pkg_updater ps_background ui ui_features companion_features i18n common/graphics common/controller common/png)
APP_OBJS=()
for s in "${APP_SRCS[@]}"; do
    n="$(basename "$s")"
    echo "  $n.cpp"
    "$CLANGXX" "${CXXFLAGS[@]}" -o "$APP_DIR/obj/$n.o" "$APP_DIR/src/$s.cpp"
    APP_OBJS+=("$APP_DIR/obj/$n.o")
done
"$LLD" -m elf_x86_64 -pie --script "$TOOLCHAIN/link.x" --eh-frame-hdr -L"$TOOLCHAIN/lib" "$TOOLCHAIN/lib/crt1.o" \
    "${APP_OBJS[@]}" -o "$APP_DIR/bin/app.elf" \
    -lc -lkernel -lc++ -lSceVideoOut -lSceSysmodule -lSceFreeType -lScePad -lSceUserService \
    -lSceSystemService -lSceNet -lSceNetCtl -lSceSsl -lSceHttp
"$CREATE_FSELF" -in "$APP_DIR/bin/app.elf" -out "$APP_DIR/bin/app.oelf" -eboot "$APP_DIR/eboot.bin" -paid 0x3800000000000011

echo "=== Building PKG ==="
(cd "$APP_DIR" && "$PKGTOOL" pkg_build pkg.gp4 . && "$PKGTOOL" pkg_validate "$CONTENT_ID.pkg")
cp -f "$APP_DIR/$CONTENT_ID.pkg" "$PROJECT/bin/omnipad_companion.pkg"
echo "=== Done: bin/omnipad_companion.pkg ==="

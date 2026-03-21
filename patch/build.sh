#!/bin/bash
# Build the 32-bit WING32.dll shim for Tonka Construction
# Requires: mingw-w64-i686-gcc (pacman -S mingw-w64-i686-gcc)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GCC=/c/msys64/mingw32/bin/gcc.exe

if [ ! -f "$GCC" ]; then
    echo "ERROR: 32-bit MinGW GCC not found at $GCC"
    echo "Install with: pacman -S mingw-w64-i686-gcc"
    exit 1
fi

export PATH="/c/msys64/mingw32/bin:/c/msys64/usr/bin:$PATH"

echo "Building 32-bit WING32.dll..."
"$GCC" -shared \
    -o "$SCRIPT_DIR/WING32.dll" \
    "$SCRIPT_DIR/wing32.c" \
    "$SCRIPT_DIR/wing32.def" \
    -lgdi32 -luser32 \
    -Wl,--enable-stdcall-fixup \
    -O2

echo "Built: $SCRIPT_DIR/WING32.dll"
ls -la "$SCRIPT_DIR/WING32.dll"

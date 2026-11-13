#!/bin/sh
# Build XPSH.dll, reg.exe and BZDPatcher_Win2000_Setup.exe.
# Needs MinGW-w64 32-bit

set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-dist}"
mkdir -p "$OUT/tmp"
OUT="$(cd "$OUT" && pwd)"
TMP="$OUT/tmp"
GEN="$ROOT/src/shim/generated"

i686-w64-mingw32-gcc -c -O2 -Wall -I"$GEN" -o "$TMP/xpshim.o" "$ROOT/src/shim/xpshim.c"
i686-w64-mingw32-as -o "$TMP/stubs.o" "$GEN/stubs.s"
i686-w64-mingw32-gcc -shared -O2 -o "$OUT/XPSH.dll" "$TMP/xpshim.o" "$TMP/stubs.o" "$GEN/exports.def" \
    -Wl,--kill-at \
    -Wl,--major-os-version=5 -Wl,--minor-os-version=0 \
    -Wl,--major-subsystem-version=5 -Wl,--minor-subsystem-version=0 \
    -static-libgcc -luser32 -lkernel32

i686-w64-mingw32-gcc -O2 -Wall -static -o "$OUT/reg.exe" "$ROOT/src/reg/reg.c"

cp "$OUT/XPSH.dll" "$OUT/reg.exe" "$TMP/"
cp "$ROOT/src/setup/setup.rc" "$ROOT/src/setup/setup.c" "$TMP/"
cd "$TMP"
i686-w64-mingw32-windres -O coff -o setup_res.o setup.rc
i686-w64-mingw32-gcc -O2 -Wall -mwindows -static -o "$OUT/BZDPatcher_Win2000_Setup.exe" setup.c setup_res.o \
    -Wl,--major-os-version=5 -Wl,--minor-os-version=0 \
    -Wl,--major-subsystem-version=5 -Wl,--minor-subsystem-version=0 \
    -luser32 -lkernel32 -lcomdlg32 -lshell32 -lole32 -loleaut32

rm -rf "$TMP"
ls -l "$OUT"

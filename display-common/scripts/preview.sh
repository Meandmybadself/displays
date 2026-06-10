#!/usr/bin/env bash
# Host-side display preview — render an app's real view + the shared screens to a
# PNG, no hardware. Compiles the library's gfx against a GFXcanvas16 (via the sim/
# shim) together with the app's view.cpp and sim/main.cpp.
#
# Apps invoke this through a thin wrapper:  scripts/preview.sh -- [view args...]
# which execs:  ../display-common/scripts/preview.sh "<app dir>" -- [view args...]
#
# Everything after a literal `--` is passed to the preview binary, e.g.:
#   scripts/preview.sh -- --screen status --cond "Connecting wifi"
set -euo pipefail

LIB_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP_DIR="$(cd "${1:-$PWD}" && pwd)"
shift || true

CXX="${CXX:-clang++}"
GFXLIB="$(find "$APP_DIR/.pio/libdeps" -maxdepth 2 -type d -iname 'Adafruit GFX Library' 2>/dev/null | head -1)"
[ -n "$GFXLIB" ] || { echo "ERROR: Adafruit GFX Library not found under $APP_DIR/.pio. Run 'pio run' once first."; exit 1; }

[ -f "$APP_DIR/sim/main.cpp" ] || { echo "ERROR: $APP_DIR/sim/main.cpp not found (the app's preview harness)."; exit 1; }

BUILD="$APP_DIR/sim/build"
mkdir -p "$BUILD"

INCLUDES=(-I "$LIB_DIR/sim/shim" -I "$LIB_DIR/src" -I "$APP_DIR/src" -I "$LIB_DIR/sim" -I "$GFXLIB")
FLAGS=(-std=c++17 -O1 -DHOST_SIM -DARDUINO=100 -Wall -Wno-unused-variable)

# Adafruit_GFX.cpp is the slow one; cache its object and only rebuild if it changed.
if [ ! -f "$BUILD/Adafruit_GFX.o" ] || [ "$GFXLIB/Adafruit_GFX.cpp" -nt "$BUILD/Adafruit_GFX.o" ]; then
    echo "Compiling Adafruit_GFX (cached after first run)..."
    "$CXX" "${FLAGS[@]}" "${INCLUDES[@]}" -c "$GFXLIB/Adafruit_GFX.cpp" -o "$BUILD/Adafruit_GFX.o"
fi

echo "Compiling preview..."
"$CXX" "${FLAGS[@]}" "${INCLUDES[@]}" \
    "$APP_DIR/sim/main.cpp" \
    "$LIB_DIR/src/gfx.cpp" \
    "$LIB_DIR/src/gfx_fonts.cpp" \
    "$APP_DIR/src/view.cpp" \
    "$LIB_DIR/sim/shim/Print.cpp" \
    "$BUILD/Adafruit_GFX.o" \
    -o "$BUILD/preview"

# Pass through any args after a literal `--`.
EXTRA=()
seen=0
for a in "$@"; do
    if [ "$seen" = 1 ]; then EXTRA+=("$a"); fi
    if [ "$a" = "--" ]; then seen=1; fi
done

OUT="$APP_DIR/preview.png"
"$BUILD/preview" --out "$OUT" ${EXTRA[@]+"${EXTRA[@]}"}
open "$OUT"

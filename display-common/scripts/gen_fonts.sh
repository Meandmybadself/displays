#!/usr/bin/env bash
# Generate the anti-aliased Helvetica Neue glyph tables for a display app.
#
#   - CORE faces (8/10/20/30pt Bold) used by the shared screens are written into
#     display-common/src/fonts/ (generated once; identical for every app).
#   - APP faces (the data view's own sizes + any icon fonts) are written into
#     <app>/src/fonts/ by the app's scripts/app_fonts.sh, which defines a
#     gen_app_fonts() function we source and call below.
#
# Apps invoke this through a thin wrapper:  scripts/gen_fonts.sh
# which execs:  ../display-common/scripts/gen_fonts.sh "<app dir>"
#
# Requirements (host/macOS):
#   - HelveticaNeue.ttc in the app (or library) root
#       macOS: cp /System/Library/Fonts/HelveticaNeue.ttc .
#   - freetype:  brew install freetype
#   - fonttools: pip3 install fonttools
set -euo pipefail

LIB_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP_DIR="$(cd "${1:-$PWD}" && pwd)"
CC="${CC:-/usr/bin/cc}"   # the shell's `cc` is sometimes shadowed; use the real compiler

# HelveticaNeue.ttc: prefer the app's copy, fall back to the library's.
TTC=""
for cand in "$APP_DIR/HelveticaNeue.ttc" "$LIB_DIR/HelveticaNeue.ttc"; do
    [ -f "$cand" ] && { TTC="$cand"; break; }
done
[ -n "$TTC" ] || { echo "ERROR: HelveticaNeue.ttc not found in $APP_DIR or $LIB_DIR"; \
    echo "  macOS: cp /System/Library/Fonts/HelveticaNeue.ttc \"$APP_DIR\""; exit 1; }

FT="$(brew --prefix freetype 2>/dev/null || true)"
[ -n "$FT" ] && [ -d "$FT" ] || { echo "ERROR: freetype not found. Install: brew install freetype"; exit 1; }
python3 -c "import fontTools" 2>/dev/null || { echo "ERROR: fonttools missing. Install: pip3 install fonttools"; exit 1; }

WORK="$LIB_DIR/.fonttool"
CORE_OUT="$LIB_DIR/src/fonts"
mkdir -p "$WORK" "$CORE_OUT"

echo "Building aa_fontconvert (anti-aliased 4-bit glyph generator)..."
"$CC" "$LIB_DIR/scripts/aa_fontconvert.c" -I"$FT/include/freetype2" -L"$FT/lib" -lfreetype -o "$WORK/aa_fontconvert"
FCV="$WORK/aa_fontconvert"

echo "Extracting the Bold face from $TTC..."
# fontconvert only reads face 0 of a file, so split the Bold weight (face 1) out first.
python3 - "$TTC" "$WORK" <<'PY'
import sys
from fontTools.ttLib import TTCollection
ttc, work = sys.argv[1], sys.argv[2]
c = TTCollection(ttc)
c.fonts[1].save(f"{work}/HelveticaNeueBold.ttf")  # face 1 = Helvetica Neue Bold
PY
BOLD_TTF="$WORK/HelveticaNeueBold.ttf"

echo "Generating CORE faces into $CORE_OUT/ ..."
# aa_fontconvert renders at 141 DPI, so pixel-em ~= point-size * 1.96.
"$FCV" "$BOLD_TTF"  8        > "$CORE_OUT/HelveticaNeueBold8pt7b.h"    # hints / status bar
"$FCV" "$BOLD_TTF"  10       > "$CORE_OUT/HelveticaNeueBold10pt7b.h"   # headers / boot screens
"$FCV" "$BOLD_TTF"  20       > "$CORE_OUT/HelveticaNeueBold20pt7b.h"   # heavy status screens
"$FCV" "$BOLD_TTF"  30 42 58 > "$CORE_OUT/HelveticaNeueBold30pt7b.h"   # '*'..':' countdown digit

# --- App-specific faces -------------------------------------------------------
# The app declares its own sizes (and any icon fonts) in scripts/app_fonts.sh,
# which defines gen_app_fonts(). We export the tools it needs.
export FCV BOLD_TTF WORK APP_DIR
export OUT="$APP_DIR/src/fonts"
mkdir -p "$OUT"

if [ -f "$APP_DIR/scripts/app_fonts.sh" ]; then
    echo "Generating APP faces into $OUT/ ..."
    # shellcheck disable=SC1090
    source "$APP_DIR/scripts/app_fonts.sh"
    gen_app_fonts
else
    echo "NOTE: $APP_DIR/scripts/app_fonts.sh not found — skipping app-specific faces."
fi

echo "Done."
ls -l "$CORE_OUT"/HelveticaNeueBold*pt7b.h "$OUT"/*pt7b.h 2>/dev/null || true

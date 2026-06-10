#pragma once
//
// The shared design-language type faces: Helvetica Neue Bold, as anti-aliased
// 4-bit-alpha fonts (AAfont format, see aa_font.h). These four sizes are what the
// generic screens in gfx.cpp render with, so they live in the library and are
// generated once into src/fonts/ by scripts/gen_fonts.sh.
//
// The generated glyph tables have no include guards, so they may only be pulled
// into ONE translation unit — that's gfx_fonts.cpp. Every other file (gfx.cpp and
// each app's view) refers to a face through these extern handles instead, and
// passes the AAfont* to gfx::text(). App-specific faces (big temperature, weather
// icons, price digits) are owned by the app's own view translation unit the same way.
//
#include "fonts/aa_font.h"

namespace gfx {
    extern const AAfont* const FONT_SMALL;   // 8pt  — hints, footers, status bar
    extern const AAfont* const FONT_MED;     // 10pt — headers, boot screens
    extern const AAfont* const FONT_STATUS;  // 20pt — heavy bottom-left status screens
    extern const AAfont* const FONT_BIG;     // 30pt — factory-reset countdown digit
}

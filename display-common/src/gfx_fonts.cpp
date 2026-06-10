// The single translation unit that owns the core Helvetica Neue glyph tables and
// publishes them as external handles (see gfx_fonts.h for why this is isolated).
#include <Arduino.h>      // PROGMEM, used by the generated glyph tables
#include "gfx_fonts.h"
#include "fonts/core_fonts.h"

namespace gfx {
    extern const AAfont* const FONT_SMALL  = &HelveticaNeueBold8pt7b;
    extern const AAfont* const FONT_MED    = &HelveticaNeueBold10pt7b;
    extern const AAfont* const FONT_STATUS = &HelveticaNeueBold20pt7b;
    extern const AAfont* const FONT_BIG    = &HelveticaNeueBold30pt7b;
}

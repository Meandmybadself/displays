#include "view.h"
#include "config.h"            // COLOR_*, TFT_* (via dd_config.h)
#include "weather_fetcher.h"   // WeatherSnapshot

#include <gfx.h>
#include <Arduino.h>
#include <math.h>

// App-specific font faces (single translation unit — see fonts/app_fonts.h).
#include "fonts/app_fonts.h"

namespace {
    const AAfont* const FONT_COL  = &HelveticaNeueBold9_45pt7b;  // right-column items
    const AAfont* const FONT_TEMP = &HelveticaNeueBold52pt7b;    // full-height temperature
    const AAfont* const FONT_ICON = &WeatherIcons12_23pt7b;      // condition glyph
    const AAfont* const FONT_RAIN = &WeatherIcons9pt7b;          // small rain glyph for precip %

    // Map an OpenWeatherMap icon code (e.g. "10d") to a Weather Icons glyph, which
    // gen_fonts.sh / app_fonts.sh remapped onto ASCII 'A'..'K'. Keep this in sync
    // with the MAP in scripts/app_fonts.sh. Returns 0 when there's no sensible glyph.
    char owm_icon_glyph(const String& code) {
        if (code.length() < 3) return 0;
        bool day = code.charAt(2) != 'n';
        if (code.charAt(0) == '0') {
            switch (code.charAt(1)) {
                case '1': return day ? 'A' : 'B';   // clear sky
                case '2': return day ? 'C' : 'D';   // few clouds
                case '3': return 'E';               // scattered clouds
                case '4': return 'E';               // broken / overcast clouds
                case '9': return 'F';               // shower rain
            }
        } else if (code.charAt(0) == '1') {
            switch (code.charAt(1)) {
                case '0': return day ? 'G' : 'H';   // rain
                case '1': return 'I';               // thunderstorm
                case '3': return 'J';               // snow
            }
        } else if (code.charAt(0) == '5') {
            return 'K';                             // mist / fog
        }
        return 0;
    }

    // Draw a single icon glyph, its ink horizontally centred on `cx` with the
    // visual top at `top`.
    void draw_icon_top(char glyph, int16_t cx, int16_t top, uint16_t color) {
        if (!glyph) return;
        char buf[2] = { glyph, 0 };
        int16_t x1, y1; uint16_t w, h;
        gfx::text_bounds(FONT_ICON, buf, &x1, &y1, &w, &h);
        gfx::text(FONT_ICON, cx - (int16_t)w / 2 - x1, top - y1, buf, color, COLOR_BG);
    }
}

namespace view {

void draw(const WeatherSnapshot& s) {
    gfx::clear();

    // Big temperature, left-justified and vertically centred, filling the height.
    char tbuf[8];
    snprintf(tbuf, sizeof(tbuf), "%.0f", s.temp_f);
    String temp = tbuf;
    if (s.stale) temp += "*";

    constexpr int16_t TEMP_X = 8;
    int16_t  x1, y1;
    uint16_t w, h;
    gfx::text_bounds(FONT_TEMP, temp, &x1, &y1, &w, &h);
    int16_t top = (TFT_HEIGHT - (int16_t)h) / 2;
    gfx::text(FONT_TEMP, TEMP_X - x1, top - y1, temp, COLOR_FG, COLOR_BG);

    // Degree sign, drawn as an anti-aliased ring just off the digits' top-right.
    // The temperature font carries only '*'..':' (no '°' glyph), so render it geometrically.
    constexpr float   DEG_R     = 6.3f;  // outer radius (5% larger than the original 6px)
    constexpr float   DEG_THICK = 3.15f; // bold stroke, scaled 5% to match
    constexpr int16_t DEG_GAP   = 6;     // horizontal gap from the digits
    int16_t deg_r  = (int16_t)(DEG_R + 0.5f);
    int16_t deg_cx = TEMP_X + (int16_t)w + DEG_GAP + deg_r;
    int16_t deg_cy = top + deg_r + 4;
    gfx::ring(deg_cx, deg_cy, DEG_R, DEG_THICK, COLOR_FG, COLOR_BG);

    // Right column, centred in the space beside the temperature:
    //   condition icon at the top, low/high beneath it, precip at the bottom.
    constexpr int16_t COL_X  = 150;
    constexpr int16_t COL_W  = TFT_WIDTH - COL_X;        // 90px
    constexpr int16_t COL_CX = COL_X + COL_W / 2;        // column centre x

    char hl[20], pct[8];
    snprintf(hl,  sizeof(hl),  "%.0f / %.0f", s.low_f, s.high_f);          // low / high
    snprintf(pct, sizeof(pct), "%d%%", (int)roundf(s.pop * 100.0f));       // precip chance

    // Top-align the condition icon with the temperature digits' visual top.
    char gbuf[2] = { owm_icon_glyph(s.icon_code), 0 };
    int16_t gx1, gy1; uint16_t gw, gh;
    gfx::text_bounds(FONT_ICON, gbuf, &gx1, &gy1, &gw, &gh);
    draw_icon_top(gbuf[0], COL_CX, top, COLOR_ACCENT);
    int16_t icon_bottom = top + (int16_t)gh;

    int16_t temp_bottom = top + (int16_t)h;

    int16_t ix1, iy1; uint16_t iw, ih;            // rain glyph
    int16_t px1, py1; uint16_t pw, ph;            // precip %
    int16_t hx1, hy1; uint16_t hw, hh;            // high / low
    gfx::text_bounds(FONT_RAIN, "L", &ix1, &iy1, &iw, &ih);
    gfx::text_bounds(FONT_COL,  pct, &px1, &py1, &pw, &ph);
    gfx::text_bounds(FONT_COL,  hl,  &hx1, &hy1, &hw, &hh);

    // Precip row: rain glyph + percentage, left-aligned, both bottoms on the
    // temperature's bottom line.
    constexpr int16_t PRECIP_GAP = 4;             // glyph-to-text gap
    gfx::text(FONT_RAIN, COL_X - ix1, temp_bottom - (int16_t)ih - iy1 + 4, "L", COLOR_DIM, COLOR_BG);
    int16_t pct_x = COL_X + (int16_t)iw + PRECIP_GAP;
    gfx::text(FONT_COL, pct_x - px1, temp_bottom - (int16_t)ph - py1, pct, COLOR_DIM, COLOR_BG);

    // High / low, vertically centred in the gap between icon bottom and precip top.
    int16_t precip_top = temp_bottom - (ih > ph ? (int16_t)ih : (int16_t)ph);
    int16_t hl_top     = icon_bottom + (precip_top - icon_bottom - (int16_t)hh) / 2;
    gfx::left_top(FONT_COL, COL_X, hl_top, COLOR_FG, hl);
}

} // namespace view

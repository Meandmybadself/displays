#include "view.h"
#include "config.h"          // COLOR_*, TFT_* (via dd_config.h)
#include "stock_fetcher.h"   // StockQuote

#include <gfx.h>
#include <Arduino.h>
#include <math.h>

// App-specific font faces (single translation unit — see fonts/app_fonts.h).
#include "fonts/app_fonts.h"

namespace {
    const AAfont* const FONT_PRICE_BIG   = &HelveticaNeueBold41pt7b;  // normal headline
    const AAfont* const FONT_PRICE_SMALL = &HelveticaNeueBold29pt7b;  // wide-price fallback
    const AAfont* const FONT_COL         = &HelveticaNeueBold15pt7b;  // symbol + % change

    // Draw `s` right-aligned: its right ink edge sits at `right_x`, visual top at `top`.
    void right_top(const AAfont* font, int16_t right_x, int16_t top, uint16_t color, const String& s) {
        int16_t x1, y1; uint16_t w, h;
        gfx::text_bounds(font, s, &x1, &y1, &w, &h);
        gfx::text(font, right_x - (int16_t)w - x1, top - y1, s, color, COLOR_BG);
    }

    String format_price(float price) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", price);
        return String(buf);
    }
    String format_pct(float pct) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%c%.2f%%", (pct >= 0 ? '+' : '-'), fabsf(pct));
        return String(buf);
    }
}

namespace view {

// Layout mirrors the weather screen: a big left-justified, vertically-centred
// headline (the price, like the temperature) and a small right column (the symbol
// and % change, like the weather icon / high-low / precip rows).
void draw(const StockQuote& q) {
    gfx::clear();

    String price = format_price(q.price);
    String sym   = q.symbol;
    if (q.stale) sym += " *";
    String pct   = format_pct(q.change_pct);

    constexpr int16_t PRICE_X = 8;
    constexpr int16_t RIGHT_X = TFT_WIDTH - 6;  // right column hugs this edge
    constexpr int16_t GAP     = 8;              // min gap between price and column

    // The right column's left boundary = the wider of the symbol / % change.
    int16_t  sx1, sy1, cx1, cy1;
    uint16_t sw, sh, cw, ch;
    gfx::text_bounds(FONT_COL, sym, &sx1, &sy1, &sw, &sh);
    gfx::text_bounds(FONT_COL, pct, &cx1, &cy1, &cw, &ch);
    int16_t col_left = RIGHT_X - (int16_t)(sw > cw ? sw : cw);

    // Big left-justified, vertically-centred price (like the weather temperature),
    // dropping to the smaller face if it would otherwise collide with the column.
    const AAfont* face = FONT_PRICE_BIG;
    int16_t  x1, y1;
    uint16_t w, h;
    gfx::text_bounds(face, price, &x1, &y1, &w, &h);
    if (PRICE_X + (int16_t)w > col_left - GAP) {
        face = FONT_PRICE_SMALL;
        gfx::text_bounds(face, price, &x1, &y1, &w, &h);
    }
    int16_t top = (TFT_HEIGHT - (int16_t)h) / 2;
    gfx::text(face, PRICE_X - x1, top - y1, price, COLOR_FG, COLOR_BG);

    // Right column: symbol over % change, as a vertically-centred pair hugging the
    // right edge (kept a fixed gap apart so they don't collide at this larger size).
    constexpr int16_t COL_GAP = 6;
    int16_t pair_h   = (int16_t)sh + COL_GAP + (int16_t)ch;
    int16_t pair_top = (TFT_HEIGHT - pair_h) / 2;
    right_top(FONT_COL, RIGHT_X, pair_top, COLOR_DIM, sym);
    uint16_t color = q.change_pct >= 0.0f ? COLOR_UP : COLOR_DOWN;
    right_top(FONT_COL, RIGHT_X, pair_top + (int16_t)sh + COL_GAP, color, pct);
}

} // namespace view

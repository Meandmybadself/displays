#include "view.h"
#include "config.h"          // COLOR_*, TFT_* (via dd_config.h)
#include "stock_fetcher.h"   // StockQuote

#include <gfx.h>
#include <gfx_fonts.h>       // gfx::FONT_SMALL (core face) for the clock
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <math.h>

// App-specific font faces (single translation unit — see fonts/app_fonts.h).
#include "fonts/app_fonts.h"

namespace {
    const AAfont* const FONT_SYM   = &HelveticaNeueBold15pt7b;  // symbol + % change
    const AAfont* const FONT_PRICE = &HelveticaNeueBold26pt7b;  // big price

    constexpr int16_t STATUS_BAR_H = 16;

    String format_price(float price) {
        char buf[16];
        snprintf(buf, sizeof(buf), "$%.0f", price);
        return String(buf);
    }
    String format_pct(float pct) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%c%.2f%%", (pct >= 0 ? '+' : '-'), fabsf(pct));
        return String(buf);
    }

    // Erase the body region (everything below the status bar).
    void clear_body() {
        gfx::surface()->fillRect(0, STATUS_BAR_H, TFT_WIDTH, TFT_HEIGHT - STATUS_BAR_H, COLOR_BG);
    }
}

namespace view {

void draw(const StockQuote& q) {
    clear_body();

    // Symbol top.
    String sym = q.symbol;
    if (q.stale) sym += " *";
    gfx::center_text(FONT_SYM, STATUS_BAR_H + 8, COLOR_DIM, sym);

    // Price middle (huge).
    gfx::center_text(FONT_PRICE, 52, COLOR_FG, format_price(q.price));

    // % change bottom, colour-coded.
    uint16_t color = q.change_pct >= 0.0f ? COLOR_UP : COLOR_DOWN;
    gfx::center_text(FONT_SYM, TFT_HEIGHT - 28, color, format_pct(q.change_pct));
}

void draw_status_bar(bool wifi_ok, const String& hhmm) {
    gfx::surface()->fillRect(0, 0, TFT_WIDTH, STATUS_BAR_H, COLOR_BG);
    if (hhmm.length()) {
        gfx::left_top(gfx::FONT_SMALL, 4, 3, COLOR_DIM, hhmm);
    }
    gfx::draw_wifi_indicator(wifi_ok);
}

} // namespace view

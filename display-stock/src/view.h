#pragma once

#include <Arduino.h>

struct StockQuote;  // fwd

namespace view {
    // The stock data screen: symbol, big $price, colour-coded % change. Drawn with
    // the shared gfx primitives + this app's own font faces, below a 16px status bar.
    void draw(const StockQuote& q);

    // Top status bar: HH:MM clock (left) + the shared wifi dot (right).
    void draw_status_bar(bool wifi_ok, const String& hhmm);
}

#pragma once

struct StockQuote;  // fwd

namespace view {
    // The stock data screen: big left-justified $price (like the weather
    // temperature) with the symbol + colour-coded % change in a small right column.
    // Drawn with the shared gfx primitives + this app's own price face.
    void draw(const StockQuote& q);
}

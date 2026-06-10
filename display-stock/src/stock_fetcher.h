#pragma once

#include <Arduino.h>
#include "config.h"

struct StockQuote {
    String   symbol;
    float    price;
    float    change_pct;
    bool     valid;       // true if we have ever successfully fetched
    bool     stale;       // true if last successful fetch is older than STALE_THRESHOLD_MS
    uint32_t fetched_at;  // millis() of last successful fetch
};

namespace stock_fetcher {
    void begin(const String symbols[], uint8_t count);

    // Fetches one symbol per call (round-robins through the list). Returns true
    // when a full cycle has completed (so the caller can schedule the next one).
    // Splitting per-iteration keeps loop() under the ESP32 task watchdog.
    bool refresh_next();

    bool has_any_valid();
    const StockQuote& quote(uint8_t i);

    // Find the next/prev index whose quote is valid; returns -1 if none.
    int next_valid_index(int from);
    int prev_valid_index(int from);

    // Mark cached quotes as `stale` if older than threshold (call from main loop).
    void update_staleness();
}

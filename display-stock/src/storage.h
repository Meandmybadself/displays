#pragma once

#include <Arduino.h>
#include "config.h"

struct StockConfig {
    String   symbols_csv;
    uint16_t dwell_s;
    uint8_t  symbol_count;
    String   symbols[MAX_SYMBOLS];
};

namespace storage {
    void begin();
    bool load(StockConfig& out);
    void save(const String& symbols_csv, uint16_t dwell_s);
    void clear();
    uint8_t parse_symbols(const String& csv, String out[], uint8_t max);
}

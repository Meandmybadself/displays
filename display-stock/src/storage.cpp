#include "storage.h"
#include <Preferences.h>

namespace {
    Preferences prefs;
    constexpr const char* NS = "stock";
    constexpr const char* KEY_SYMBOLS = "symbols";
    constexpr const char* KEY_DWELL   = "dwell";

    String upper_trim(const String& in) {
        String s = in;
        s.trim();
        s.toUpperCase();
        return s;
    }
}

namespace storage {

void begin() {
    prefs.begin(NS, false);
}

bool load(StockConfig& out) {
    out.symbols_csv = prefs.getString(KEY_SYMBOLS, "");
    out.dwell_s    = prefs.getUShort(KEY_DWELL, 0);
    if (out.symbols_csv.length() == 0 || out.dwell_s == 0) {
        return false;
    }
    out.symbol_count = parse_symbols(out.symbols_csv, out.symbols, MAX_SYMBOLS);
    return out.symbol_count > 0;
}

void save(const String& symbols_csv, uint16_t dwell_s) {
    if (dwell_s < MIN_DWELL_S) dwell_s = MIN_DWELL_S;
    if (dwell_s > MAX_DWELL_S) dwell_s = MAX_DWELL_S;
    prefs.putString(KEY_SYMBOLS, symbols_csv);
    prefs.putUShort(KEY_DWELL, dwell_s);
}

void clear() {
    prefs.clear();
}

uint8_t parse_symbols(const String& csv, String out[], uint8_t max) {
    uint8_t count = 0;
    int start = 0;
    while (start <= (int)csv.length() && count < max) {
        int comma = csv.indexOf(',', start);
        String token = (comma < 0) ? csv.substring(start) : csv.substring(start, comma);
        String sym = upper_trim(token);
        if (sym.length() > 0 && sym.length() <= MAX_SYMBOL_LEN) {
            out[count++] = sym;
        }
        if (comma < 0) break;
        start = comma + 1;
    }
    return count;
}

} // namespace storage

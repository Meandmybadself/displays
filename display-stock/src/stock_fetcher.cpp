#include "stock_fetcher.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace {
    StockQuote quotes[MAX_SYMBOLS];
    uint8_t    quote_count   = 0;
    uint8_t    fetch_cursor  = 0;

    bool fetch_one(StockQuote& q) {
        if (WiFi.status() != WL_CONNECTED) return false;

        WiFiClientSecure client;
        client.setInsecure();           // hobby device on LAN; no cert pinning
        client.setTimeout(HTTP_TIMEOUT_MS / 1000);

        char path[96];
        snprintf(path, sizeof(path), YAHOO_PATH_FMT, q.symbol.c_str());
        char url[160];
        snprintf(url, sizeof(url), "https://%s%s", YAHOO_HOST, path);

        HTTPClient http;
        http.setTimeout(HTTP_TIMEOUT_MS);
        if (!http.begin(client, url)) {
            return false;
        }
        http.addHeader("User-Agent", USER_AGENT);
        http.addHeader("Accept", "application/json");

        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            log_w("HTTP %d for %s", code, q.symbol.c_str());
            http.end();
            return false;
        }

        JsonDocument filter;
        filter["chart"]["result"][0]["meta"]["regularMarketPrice"] = true;
        filter["chart"]["result"][0]["meta"]["chartPreviousClose"] = true;
        filter["chart"]["error"] = true;

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, http.getStream(),
                                                   DeserializationOption::Filter(filter));
        http.end();
        if (err) {
            log_w("JSON parse failed for %s: %s", q.symbol.c_str(), err.c_str());
            return false;
        }
        if (!doc["chart"]["error"].isNull()) {
            log_w("Yahoo error for %s", q.symbol.c_str());
            return false;
        }

        JsonVariant meta = doc["chart"]["result"][0]["meta"];
        if (meta.isNull() || meta["regularMarketPrice"].isNull() || meta["chartPreviousClose"].isNull()) {
            log_w("Missing fields for %s", q.symbol.c_str());
            return false;
        }

        float price = meta["regularMarketPrice"].as<float>();
        float prev  = meta["chartPreviousClose"].as<float>();
        if (prev <= 0.0f) return false;

        q.price       = price;
        q.change_pct  = (price - prev) / prev * 100.0f;
        q.valid       = true;
        q.stale       = false;
        q.fetched_at  = millis();
        return true;
    }
}

namespace stock_fetcher {

void begin(const String symbols[], uint8_t count) {
    quote_count = (count > MAX_SYMBOLS) ? MAX_SYMBOLS : count;
    fetch_cursor = 0;
    for (uint8_t i = 0; i < quote_count; ++i) {
        quotes[i] = {};
        quotes[i].symbol = symbols[i];
    }
}

bool refresh_next() {
    if (quote_count == 0) return true;
    fetch_one(quotes[fetch_cursor]);
    fetch_cursor++;
    if (fetch_cursor >= quote_count) {
        fetch_cursor = 0;
        return true;
    }
    return false;
}

bool has_any_valid() {
    for (uint8_t i = 0; i < quote_count; ++i) {
        if (quotes[i].valid) return true;
    }
    return false;
}

const StockQuote& quote(uint8_t i) { return quotes[i]; }

int next_valid_index(int from) {
    if (quote_count == 0) return -1;
    for (uint8_t step = 1; step <= quote_count; ++step) {
        int i = (from + step) % quote_count;
        if (quotes[i].valid) return i;
    }
    return -1;
}

int prev_valid_index(int from) {
    if (quote_count == 0) return -1;
    for (uint8_t step = 1; step <= quote_count; ++step) {
        int i = (from - (int)step + quote_count * 2) % quote_count;
        if (quotes[i].valid) return i;
    }
    return -1;
}

void update_staleness() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < quote_count; ++i) {
        if (quotes[i].valid) {
            quotes[i].stale = (now - quotes[i].fetched_at) > STALE_THRESHOLD_MS;
        }
    }
}

} // namespace stock_fetcher

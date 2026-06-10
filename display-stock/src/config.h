#pragma once
//
// Stock-app constants. Universal hardware/design constants (pins, geometry,
// colours, HTTP timeout, WiGLE, boot-hold) live in the shared library's
// <dd_config.h>, included below.
//
#include <dd_config.h>

// --- Captive portal AP name (Stage 1) ---
constexpr const char* PORTAL_AP_SSID = "StockTicker-Setup";

// --- Stage-2 setup server ---
constexpr const char* MDNS_HOSTNAME = "stockticker";   // → stockticker.local

// --- Defaults the setup form pre-fills ---
constexpr const char* DEFAULT_SYMBOLS = "AAPL,MSFT,GOOGL,NVDA,TSLA";
constexpr const char* DEFAULT_DWELL_S = "8";

// --- Bounds ---
constexpr uint8_t  MAX_SYMBOLS       = 16;
constexpr uint8_t  MAX_SYMBOL_LEN    = 10;
constexpr uint16_t MIN_DWELL_S       = 2;
constexpr uint16_t MAX_DWELL_S       = 600;

// --- Network / data ---
constexpr uint32_t REFRESH_INTERVAL_MS = 60UL * 1000UL;
constexpr uint32_t STALE_THRESHOLD_MS  = 5UL * 60UL * 1000UL;
constexpr uint32_t FETCH_STEP_GAP_MS   = 250;  // pacing between per-symbol fetches
constexpr const char* YAHOO_HOST       = "query1.finance.yahoo.com";
constexpr const char* YAHOO_PATH_FMT   = "/v8/finance/chart/%s?interval=1d&range=1d";
constexpr const char* USER_AGENT       = "Mozilla/5.0 (StockTickerESP32)";

// --- NTP ---
constexpr const char* NTP_SERVER       = "pool.ntp.org";
constexpr long NTP_OFFSET_SECONDS      = 0;  // display in UTC; user can edit if they want local
constexpr unsigned long NTP_UPDATE_MS  = 60UL * 60UL * 1000UL;  // hourly resync

// --- App-specific colours (RGB565), beyond the shared palette in dd_config.h ---
constexpr uint16_t COLOR_UP            = 0x07E0;   // green
constexpr uint16_t COLOR_DOWN          = 0xF800;   // red

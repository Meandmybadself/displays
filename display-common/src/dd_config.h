#pragma once
//
// display-common: universal design-language + hardware constants shared by every
// display app (weather, stock, ...). These are the things that DON'T change between
// apps: the board, the panel geometry, the colour palette, and the timings that
// keep HTTP work under the loopTask watchdog.
//
// App-specific constants (API hosts, portal SSID, mDNS name, refresh cadence,
// data bounds) live in each app's own src/config.h, which includes this header.
//
#include <Arduino.h>
#include <Adafruit_ST77xx.h>

// --- Board pins (Adafruit Feather ESP32-S2 Reverse TFT) ---
constexpr uint8_t PIN_TFT_BACKLIGHT  = TFT_BACKLITE;
constexpr uint8_t PIN_TFT_I2C_POWER  = TFT_I2C_POWER;
constexpr uint8_t PIN_BTN_D0         = 0;  // active LOW (BOOT button, internal pull-up)
constexpr uint8_t PIN_BTN_D1         = 1;  // active HIGH
constexpr uint8_t PIN_BTN_D2         = 2;  // active HIGH

// --- Display geometry (240x135 landscape rotation 3) ---
constexpr uint16_t TFT_WIDTH         = 240;
constexpr uint16_t TFT_HEIGHT        = 135;
constexpr uint8_t  TFT_ROTATION      = 3;

// --- Captive portal ---
constexpr uint16_t PORTAL_TIMEOUT_S  = 600;  // 10 min then reboot to retry

// --- Reset-trigger detection (hold D0 at boot) ---
constexpr uint32_t BOOT_HOLD_MS      = 2000;

// --- Wifi connect retry policy (shared by wifi_portal::connect at boot) ---
constexpr uint32_t WIFI_CONNECT_ATTEMPT_MS = 8000;   // per association attempt
constexpr uint8_t  WIFI_CONNECT_ATTEMPTS   = 4;      // attempts before falling back

// --- Network ---
// HTTP timeout MUST stay below the loopTask WDT (~5s) since fetches run from loop().
constexpr uint32_t HTTP_TIMEOUT_MS   = 4000;

// --- WiGLE geolocation (a fixed external service; same for every app) ---
constexpr const char* WIGLE_HOST       = "api.wigle.net";
constexpr const char* WIGLE_SEARCH_FMT = "/api/v2/network/search?netid=%s";
constexpr uint8_t     MAX_BSSID_QUERY  = 5;   // top-N strongest nearby networks to query
constexpr const char* DD_USER_AGENT    = "Mozilla/5.0 (ESP32Display)";  // library's own requests

// --- Colours (RGB565) — the shared palette ---
constexpr uint16_t COLOR_BG     = 0x0000;   // black
constexpr uint16_t COLOR_FG     = 0xFFFF;   // white
constexpr uint16_t COLOR_DIM    = 0x7BEF;   // gray
constexpr uint16_t COLOR_ACCENT = 0x07FF;   // cyan
constexpr uint16_t COLOR_WARN   = 0xFD20;   // amber

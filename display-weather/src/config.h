#pragma once
//
// Weather-app constants. Universal hardware/design constants (pins, geometry,
// colours, HTTP timeout, WiGLE, boot-hold) live in the shared library's
// <dd_config.h>, included below.
//
#include <dd_config.h>

// --- Captive portal AP name (Stage 1) ---
constexpr const char* PORTAL_AP_SSID = "WeatherDisplay-Setup";

// --- Defaults the setup form pre-fills ---
constexpr const char* DEFAULT_UPDATE_MIN = "30";

// --- Bounds ---
constexpr uint16_t MIN_UPDATE_MIN = 5;
constexpr uint16_t MAX_UPDATE_MIN = 240;

// --- OpenWeatherMap ---
constexpr const char* OWM_HOST     = "api.openweathermap.org";
constexpr const char* OWM_PATH_FMT =
    "/data/3.0/onecall?lat=%.6f&lon=%.6f&units=imperial&exclude=minutely,hourly&appid=%s";
constexpr const char* USER_AGENT   = "Mozilla/5.0 (WeatherDisplayESP32)";

// --- Stage-2 setup server ---
constexpr const char* MDNS_HOSTNAME = "weatherdisplay";   // → weatherdisplay.local

// --- Connection robustness (boot retry policy WIFI_CONNECT_* lives in dd_config.h) ---
// A wifi failure when we already have valid credentials + config is treated as
// transient (weak signal, AP rebooting): we retry rather than forcing re-onboarding.
constexpr uint32_t WIFI_RECONNECT_MS = 15000;  // re-issue join while disconnected in loop()
constexpr uint32_t WEATHER_RETRY_MS  = 30000;  // refetch sooner until a fetch succeeds

#include <Arduino.h>
#include <WiFi.h>
#include <cmath>
#include <map>

#include "config.h"
#include "storage.h"
#include "weather_fetcher.h"
#include "view.h"

#include <buttons.h>
#include <gfx.h>
#include <wifi_portal.h>
#include <setup_portal.h>
#include <geolocate.h>

namespace {
    WeatherConfig cfg;

    uint32_t refresh_interval_ms = 0;
    uint32_t stale_threshold_ms  = 0;
    uint32_t last_refresh_ms     = 0;
    uint32_t last_indicator_ms   = 0;
    uint32_t last_reconnect_ms   = 0;

    bool refresh_pending = true;     // run a refresh immediately at boot
    bool have_data       = false;
    bool last_fetch_ok   = false;
    bool last_wifi_ok    = false;

    void redraw() {
        if (have_data) view::draw(weather_fetcher::snapshot());
        else           gfx::show_status("Loading weather");
        gfx::draw_wifi_indicator(WiFi.status() == WL_CONNECTED);
    }

    // Stage 2: the schema-driven setup form (OWM key, optional coords/WiGLE, interval).
    [[noreturn]] void run_setup_server() {
        WeatherConfig cur;
        storage::load(cur);   // partial config is fine — we just want to prefill

        using namespace setup_portal;
        static Field fields[] = {
            { "api_key", "OpenWeatherMap API key", TEXT, false,
              "Free tier is plenty. Sign up at "
              "<a href=\"https://openweathermap.org/api\" target=\"_blank\" rel=\"noopener\">openweathermap.org/api</a>"
              " &mdash; activation can take an hour." },
            { "lat", "Latitude", TEXT, true, "Optional if you provide a WiGLE token below.", "44.9778" },
            { "lon", "Longitude", TEXT, true, nullptr, "-93.2650" },
            { "wigle", "WiGLE API token", TEXT, true,
              "\"Encoded for use\" string from "
              "<a href=\"https://wigle.net/account\" target=\"_blank\" rel=\"noopener\">wigle.net/account</a>. "
              "If lat/lon are blank, this derives coordinates from nearby wifi networks." },
            { "upmin", "Update every N minutes", NUMBER, false, nullptr, nullptr,
              MIN_UPDATE_MIN, MAX_UPDATE_MIN },
        };
        fields[0].value = cur.api_key;
        fields[1].value = std::isnan(cur.lat) ? String("") : String(cur.lat, 6);
        fields[2].value = std::isnan(cur.lon) ? String("") : String(cur.lon, 6);
        fields[3].value = cur.wigle_token;
        fields[4].value = String(cur.update_min ? cur.update_min
                                                : (uint16_t)String(DEFAULT_UPDATE_MIN).toInt());

        const char* intro =
            "<strong>You'll need:</strong> a free "
            "<a href=\"https://openweathermap.org/api\" target=\"_blank\" rel=\"noopener\">OpenWeatherMap API key</a>. "
            "Optionally, a <a href=\"https://wigle.net/account\" target=\"_blank\" rel=\"noopener\">WiGLE API token</a> "
            "auto-detects your location instead of typing coordinates.";

        run(MDNS_HOSTNAME, "WeatherDisplay setup", intro,
            fields, sizeof(fields) / sizeof(fields[0]),
            [](const std::map<String, String>& v, String& err) -> bool {
                String api   = v.at("api_key");
                String lat_s = v.at("lat");
                String lon_s = v.at("lon");
                String wig   = v.at("wigle");
                String up    = v.at("upmin");

                bool  have_coords = lat_s.length() > 0 && lon_s.length() > 0;
                float lat = have_coords ? lat_s.toFloat() : NAN;
                float lon = have_coords ? lon_s.toFloat() : NAN;
                if (have_coords && (lat < -90.0f || lat > 90.0f ||
                                    lon < -180.0f || lon > 180.0f)) {
                    err = "Latitude must be -90..90 and longitude -180..180.";
                    return false;
                }
                if (api.length() == 0 || (!have_coords && wig.length() == 0)) {
                    err = "Enter your API key, plus either latitude & longitude or a WiGLE token.";
                    return false;
                }
                uint16_t upmin = (uint16_t)up.toInt();
                if (upmin == 0) upmin = (uint16_t)String(DEFAULT_UPDATE_MIN).toInt();

                storage::save(lat, lon, api, wig, upmin);
                return true;
            });
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    buttons::begin();
    gfx::begin();
    storage::begin();

    gfx::show_boot_message("WeatherDisplay", "booting...");
    delay(300);

    wifi_portal::check_factory_reset([] { storage::clear(); });

    bool cfg_ok = storage::load(cfg);
    Serial.printf("[main] storage::load=%d  api_key.len=%u  lat=%f lon=%f  wigle.len=%u  upmin=%u\n",
                  (int)cfg_ok, (unsigned)cfg.api_key.length(),
                  cfg.lat, cfg.lon, (unsigned)cfg.wigle_token.length(),
                  (unsigned)cfg.update_min);

    // Stage 1: no wifi credentials → captive portal (wifi only).
    if (!wifi_portal::has_saved_credentials()) {
        Serial.println("[main] no saved wifi creds -> Stage 1 portal");
        wifi_portal::run_portal(PORTAL_AP_SSID, "WeatherDisplay");  // does not return
    }

    // We have saved credentials. Try hard to connect before doing anything drastic.
    gfx::show_status("Connecting wifi");
    bool wifi_ok = wifi_portal::connect(WIFI_CONNECT_ATTEMPT_MS, WIFI_CONNECT_ATTEMPTS);

    bool have_coords = !std::isnan(cfg.lat) && !std::isnan(cfg.lon);
    bool config_complete = cfg.api_key.length() > 0
                           && (have_coords || cfg.wigle_token.length() > 0);
    Serial.printf("[main] wifi_ok=%d config_complete=%d have_coords=%d\n",
                  (int)wifi_ok, (int)config_complete, (int)have_coords);

    if (!wifi_ok) {
        if (!config_complete) {
            gfx::show_boot_message("Wifi failed", "Entering setup");
            delay(1500);
            wifi_portal::run_portal(PORTAL_AP_SSID, "WeatherDisplay");  // does not return
        }
        if (!have_coords) {
            // Config complete but coords still need a WiGLE lookup, which needs wifi —
            // reboot and keep retrying (self-healing) over a transient outage.
            gfx::show_boot_message("Wifi unavailable", "retrying...");
            delay(3000);
            ESP.restart();
        }
        // Complete config AND known coordinates: treat the outage as transient and
        // run, letting loop() keep reconnecting — never force re-onboarding.
        Serial.println("[main] wifi down but config complete -> run, retry in loop()");
    }

    // Stage 2: wifi up but weather config incomplete → local setup server.
    if (wifi_ok && !config_complete) {
        Serial.println("[main] -> Stage 2 setup server");
        run_setup_server();  // does not return
    }

    refresh_interval_ms = (uint32_t)cfg.update_min * 60UL * 1000UL;
    stale_threshold_ms  = refresh_interval_ms * 2;

    // Resolve lat/lon via WiGLE if the user didn't enter coords directly.
    if (!have_coords) {
        gfx::show_boot_message("Locating...", "scanning nearby wifi");
        float lat = 0.0f, lon = 0.0f;
        if (geolocate::resolve(cfg.wigle_token, lat, lon)) {
            cfg.lat = lat;
            cfg.lon = lon;
            storage::save_location(lat, lon);
        } else {
            gfx::show_boot_message("Locate failed", "Re-enter setup");
            delay(2000);
            run_setup_server();  // does not return
        }
    }

    gfx::show_boot_message("Loading weather", nullptr);
    weather_fetcher::begin(cfg.lat, cfg.lon, cfg.api_key);

    gfx::show_status("Loading weather");
    last_refresh_ms = millis();
}

void loop() {
    uint32_t now = millis();

    // Schedule a refresh. Until a fetch succeeds (and again after any failure) retry
    // on the short interval, so a boot-time or temporary outage doesn't leave us blank.
    uint32_t due = last_fetch_ok ? refresh_interval_ms : WEATHER_RETRY_MS;
    if (!refresh_pending && now - last_refresh_ms >= due) {
        refresh_pending = true;
    }

    // Run the refresh (one HTTP request, bounded by HTTP_TIMEOUT_MS < loopTask WDT).
    if (refresh_pending) {
        bool ok = weather_fetcher::refresh();
        last_refresh_ms = now;
        refresh_pending = false;
        last_fetch_ok   = ok;
        if (ok) have_data = true;
        if (have_data) redraw();   // fresh success, or keep last data with stale marker
    }

    weather_fetcher::update_staleness(stale_threshold_ms);

    // Buttons. D0 forces a refresh; D1/D2 are unused.
    ButtonEvent ev = buttons::poll();
    if (ev == BTN_D0_PRESSED) {
        refresh_pending = true;
    }

    // Keep wifi up: when disconnected, re-issue the join periodically.
    bool wifi_ok = (WiFi.status() == WL_CONNECTED);
    if (!wifi_ok && now - last_reconnect_ms >= WIFI_RECONNECT_MS) {
        Serial.println("[wifi] disconnected -> re-issuing join");
        WiFi.disconnect(/*wifioff=*/false, /*eraseap=*/false);
        WiFi.begin();
        last_reconnect_ms = now;
    }

    // Wifi indicator tick (1Hz). Only repaint on state change so we don't flicker.
    if (now - last_indicator_ms >= 1000) {
        if (wifi_ok != last_wifi_ok) {
            gfx::draw_wifi_indicator(wifi_ok);
            last_wifi_ok = wifi_ok;
        }
        last_indicator_ms = now;
    }

    delay(10);
}

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <map>

#include "config.h"
#include "storage.h"
#include "stock_fetcher.h"
#include "view.h"

#include <buttons.h>
#include <gfx.h>
#include <wifi_portal.h>
#include <setup_portal.h>

namespace {
    StockConfig cfg;

    int      current_index    = 0;
    uint32_t last_advance_ms  = 0;
    uint32_t last_refresh_ms  = 0;
    uint32_t last_fetch_step_ms = 0;
    uint32_t last_status_ms   = 0;
    uint32_t dwell_ms         = 0;

    bool     refresh_active   = true;   // first cycle runs immediately at boot
    bool     have_data        = false;

    WiFiUDP   ntp_udp;
    NTPClient ntp(ntp_udp, NTP_SERVER, NTP_OFFSET_SECONDS, NTP_UPDATE_MS);

    void redraw_current() {
        if (have_data) view::draw(stock_fetcher::quote(current_index));
        else           gfx::show_status("Loading prices");
    }

    void advance(bool forward) {
        int next = forward
            ? stock_fetcher::next_valid_index(current_index)
            : stock_fetcher::prev_valid_index(current_index);
        if (next < 0) return;
        current_index = next;
        view::draw(stock_fetcher::quote(current_index));
    }

    // Stage 2: the schema-driven setup form (symbols + dwell).
    [[noreturn]] void run_setup_server() {
        StockConfig cur;
        storage::load(cur);   // partial config is fine — we just want to prefill

        using namespace setup_portal;
        static Field fields[] = {
            { "symbols", "Stock symbols", TEXT, false,
              "Comma-separated tickers, e.g. AAPL, MSFT, NVDA. Up to 16." },
            { "dwell_s", "Seconds per symbol", NUMBER, false, nullptr, nullptr,
              MIN_DWELL_S, MAX_DWELL_S },
        };
        fields[0].value = cur.symbols_csv.length() ? cur.symbols_csv : String(DEFAULT_SYMBOLS);
        fields[1].value = String(cur.dwell_s ? cur.dwell_s
                                             : (uint16_t)String(DEFAULT_DWELL_S).toInt());

        run(MDNS_HOSTNAME, "StockTicker setup",
            "Pick the tickers to rotate through. Prices are fetched from Yahoo Finance.",
            fields, sizeof(fields) / sizeof(fields[0]),
            [](const std::map<String, String>& v, String& err) -> bool {
                String   symbols = v.at("symbols");
                uint16_t dwell   = (uint16_t)v.at("dwell_s").toInt();

                String parsed[MAX_SYMBOLS];
                if (storage::parse_symbols(symbols, parsed, MAX_SYMBOLS) == 0) {
                    err = "Enter at least one valid ticker (max 10 characters each).";
                    return false;
                }
                if (dwell == 0) dwell = (uint16_t)String(DEFAULT_DWELL_S).toInt();

                storage::save(symbols, dwell);
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

    gfx::show_boot_message("StockTicker", "booting...");
    delay(300);

    wifi_portal::check_factory_reset([] { storage::clear(); });

    bool cfg_ok = storage::load(cfg);
    Serial.printf("[main] storage::load=%d symbols='%s' dwell=%u\n",
                  (int)cfg_ok, cfg.symbols_csv.c_str(), (unsigned)cfg.dwell_s);

    // Stage 1: no wifi credentials → captive portal (wifi only).
    if (!wifi_portal::has_saved_credentials()) {
        Serial.println("[main] no saved wifi creds -> Stage 1 portal");
        wifi_portal::run_portal(PORTAL_AP_SSID, "StockTicker");  // does not return
    }

    // We have saved credentials. Try hard to connect before doing anything drastic.
    gfx::show_status("Connecting wifi");
    bool wifi_ok = wifi_portal::connect(WIFI_CONNECT_ATTEMPT_MS, WIFI_CONNECT_ATTEMPTS);

    if (!wifi_ok) {
        // No usable config yet → onboard via the portal (the only way forward).
        if (!cfg_ok) {
            gfx::show_boot_message("Wifi failed", "Entering setup");
            delay(1500);
            wifi_portal::run_portal(PORTAL_AP_SSID, "StockTicker");  // does not return
        }
        // Config complete: treat the outage as transient, run, and let loop()
        // keep reconnecting — never force re-onboarding for a dropout.
        Serial.println("[main] wifi down but config complete -> run, retry in loop()");
    }

    // Stage 2: wifi up but symbols not configured → local setup server.
    if (wifi_ok && !cfg_ok) {
        Serial.println("[main] -> Stage 2 setup server");
        run_setup_server();  // does not return
    }

    dwell_ms = (uint32_t)cfg.dwell_s * 1000UL;

    gfx::show_status("Loading prices");
    stock_fetcher::begin(cfg.symbols, cfg.symbol_count);
    ntp.begin();
    ntp.update();

    redraw_current();

    uint32_t now = millis();
    last_advance_ms = now;
    last_refresh_ms = now;
    last_fetch_step_ms = 0;
    last_status_ms = 0;
}

void loop() {
    uint32_t now = millis();

    // Schedule a new refresh cycle.
    if (!refresh_active && now - last_refresh_ms >= REFRESH_INTERVAL_MS) {
        refresh_active = true;
    }

    // Step the refresh state machine — at most ONE HTTP request per loop iteration
    // (bounded by HTTP_TIMEOUT_MS < the loopTask WDT).
    if (refresh_active && now - last_fetch_step_ms >= FETCH_STEP_GAP_MS) {
        bool cycle_done = stock_fetcher::refresh_next();
        last_fetch_step_ms = now;

        // First successful fetch in the lifetime of the device: pivot off "loading".
        if (!have_data && stock_fetcher::has_any_valid()) {
            int first = stock_fetcher::quote(current_index).valid
                            ? current_index
                            : stock_fetcher::next_valid_index(current_index);
            if (first >= 0) current_index = first;
            have_data = true;
            redraw_current();
            last_advance_ms = now;
        }

        if (cycle_done) {
            refresh_active = false;
            last_refresh_ms = now;
            if (have_data) redraw_current();   // re-draw current so any updated price shows
        }
    }

    stock_fetcher::update_staleness();

    // Auto-advance.
    if (have_data && now - last_advance_ms >= dwell_ms) {
        advance(true);
        last_advance_ms = now;
    }

    // Buttons.
    ButtonEvent ev = buttons::poll();
    if (ev == BTN_D2_PRESSED) {
        advance(true);
        last_advance_ms = now;
    } else if (ev == BTN_D1_PRESSED) {
        advance(false);
        last_advance_ms = now;
    } else if (ev == BTN_D0_PRESSED) {
        refresh_active = true;          // force a fresh cycle
        last_fetch_step_ms = 0;
    }

    // Status bar (clock + wifi dot) once a second.
    if (now - last_status_ms >= 1000) {
        bool wifi_ok = (WiFi.status() == WL_CONNECTED);
        if (!wifi_ok) WiFi.reconnect();   // nudge ESP32 auto-reconnect when it stalls
        ntp.update();
        String hhmm;
        if (wifi_ok && ntp.getEpochTime() > 100000) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%02d:%02d", ntp.getHours(), ntp.getMinutes());
            hhmm = buf;
        }
        view::draw_status_bar(wifi_ok, hhmm);
        last_status_ms = now;
    }

    delay(10);
}

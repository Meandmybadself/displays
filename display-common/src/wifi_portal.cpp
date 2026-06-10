#include "wifi_portal.h"
#include "dd_config.h"
#include "gfx.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>
#include <cstring>
#include <vector>

namespace wifi_portal {

void check_factory_reset(std::function<void()> clear_app_config) {
    if (digitalRead(PIN_BTN_D0) != LOW) return;
    for (uint8_t s = (BOOT_HOLD_MS / 1000); s >= 1; --s) {
        gfx::show_factory_reset_countdown(s);
        uint32_t t0 = millis();
        while (millis() - t0 < 1000) {
            if (digitalRead(PIN_BTN_D0) != LOW) return;  // released early -> abort
            delay(20);
        }
    }
    gfx::show_boot_message("Resetting...", "Releasing setup mode");
    reset_wifi();
    if (clear_app_config) clear_app_config();
    delay(500);
    ESP.restart();
}

[[noreturn]] void run_portal(const char* ap_ssid, const char* title) {
    WiFiManager wm;
    String ssid = ap_ssid;   // copy for the AP callback below

    wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
    wm.setBreakAfterConfig(true);
    wm.setTitle(title);

    // Banner via body::before lets us inject a step-1-of-2 explanation onto every
    // portal page without WiFiManager template support.
    wm.setCustomHeadElement(
        "<style>"
        "body,input,button,select,textarea{font-family:'Helvetica Neue',Helvetica,Arial,sans-serif}"
        "body::before{"
        "content:'Step 1 of 2 \\2014 enter wifi only. After saving, the screen shows a URL where you finish setup.';"
        "display:block;background:#fff3cd;border:1px solid #ffeaa7;padding:0.7em;"
        "margin-bottom:1em;border-radius:4px;font-size:0.92em"
        "}"
        "</style>"
    );

    // Default WiFiManager menu is {wifi,info,param,sep,restart,exit}; we collect
    // config in Stage 2, so drop custom params and the dead-end "exit" link.
    std::vector<const char*> menu = {"wifi", "info", "sep", "restart"};
    wm.setMenu(menu);

    wm.setAPCallback([ssid](WiFiManager*) {
        gfx::show_portal_instructions(ssid.c_str(), WiFi.softAPIP());
    });

    gfx::show_boot_message("Starting setup", "AP coming up...");

    bool saved = wm.startConfigPortal(ap_ssid);

    if (saved) {
        gfx::show_boot_message("Saved.", "Restarting...");
    } else {
        gfx::show_boot_message("Setup timed out", "Restarting...");
    }

    delay(1500);
    ESP.restart();
    while (true) { delay(1000); }
}

void reset_wifi() {
    WiFiManager wm;
    wm.resetSettings();   // wipes wifi creds in NVS
}

bool connect(uint32_t per_attempt_ms, uint8_t attempts) {
    WiFi.mode(WIFI_STA);
    // Modem-sleep is the usual culprit behind stalled associations and dropped
    // pings on a weak link; keep the radio awake for a reliable desk display.
    WiFi.setSleep(false);
    // Max TX power helps the uplink complete the WPA handshake on a marginal link
    // (this device often sees the AP at only ~-76 dBm).
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    for (uint8_t a = 1; a <= attempts; ++a) {
        if (a > 1) {
            WiFi.disconnect(/*wifioff=*/false, /*eraseap=*/false);  // keep stored creds
            delay(300);
        }
        WiFi.begin();  // join using the credentials stored in NVS
        Serial.printf("[wifi] connect attempt %u/%u (timeout %lums)\n",
                      a, attempts, (unsigned long)per_attempt_ms);

        uint32_t start = millis();
        wl_status_t last = WL_IDLE_STATUS;
        while (millis() - start < per_attempt_ms) {
            wl_status_t st = WiFi.status();
            if (st != last) {
                Serial.printf("[wifi] status -> %d  rssi=%d\n", (int)st, WiFi.RSSI());
                last = st;
            }
            if (st == WL_CONNECTED) {
                Serial.printf("[wifi] connected, ip=%s rssi=%d ch=%d\n",
                              WiFi.localIP().toString().c_str(),
                              WiFi.RSSI(), WiFi.channel());
                return true;
            }
            delay(200);
        }
        Serial.printf("[wifi] attempt %u/%u failed, status=%d rssi=%d\n",
                      a, attempts, (int)WiFi.status(), WiFi.RSSI());
    }
    return false;
}

bool has_saved_credentials() {
    // Must read the PERSISTED credentials from NVS, not WiFi.SSID(): that returns
    // the *associated* AP's SSID and is empty on a fresh boot before we connect,
    // so it would send us back into the portal on every power-up. WiFi.mode() inits
    // the driver, which loads the stored station config from flash.
    WiFi.mode(WIFI_STA);
    wifi_config_t conf = {};
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) {
        Serial.println("[wifi] has_saved_credentials: esp_wifi_get_config failed");
        return false;
    }
    size_t len = strnlen(reinterpret_cast<const char*>(conf.sta.ssid),
                         sizeof(conf.sta.ssid));
    Serial.printf("[wifi] has_saved_credentials: stored SSID='%.*s' len=%u\n",
                  (int)len, conf.sta.ssid, (unsigned)len);
    return len > 0;
}

} // namespace wifi_portal

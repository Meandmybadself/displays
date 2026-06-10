#pragma once

#include <Arduino.h>
#include <functional>

namespace wifi_portal {
    // If D0 is held at boot for BOOT_HOLD_MS, show the shared countdown screen, wipe
    // the stored wifi credentials, run `clear_app_config` (the app clears its own
    // NVS), and reboot. Returns immediately if D0 isn't held, or if it's released
    // before the countdown completes (no reset). Call this first in setup().
    void check_factory_reset(std::function<void()> clear_app_config);

    // Stage 1 of onboarding. Brings up the captive-portal AP (`ap_ssid`) for wifi
    // credentials ONLY — app config is collected later by setup_portal on the real
    // network, so external signup links work. Saves creds, then reboots; never returns.
    // `title` is the WiFiManager page title (e.g. "WeatherDisplay").
    [[noreturn]] void run_portal(const char* ap_ssid, const char* title);

    // Wipes the stored wifi credentials only. The caller clears its own app config
    // (storage::clear()) so this module stays app-agnostic.
    void reset_wifi();

    // Tries to connect to the saved network. Makes up to `attempts` association
    // attempts of `per_attempt_ms` each, re-issuing the join between tries (which
    // recovers from a stalled attempt on a marginal link). Returns true once connected.
    bool connect(uint32_t per_attempt_ms, uint8_t attempts);

    // True if ESP32 NVS already has wifi credentials we can use.
    bool has_saved_credentials();
}

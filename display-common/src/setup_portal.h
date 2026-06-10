#pragma once
//
// setup_portal — Stage 2 of onboarding. One styled, Swiss-design web form served
// over mDNS on the real wifi, driven entirely by a field schema the app declares.
// Change the look here once and every app's setup page changes with it.
//
#include <Arduino.h>
#include <functional>
#include <map>

namespace setup_portal {
    enum Type { TEXT, NUMBER, PASSWORD };

    struct Field {
        const char* key;                 // form field name + map key
        const char* label;               // shown above the input
        Type        type        = TEXT;
        bool        optional     = false;// renders "(optional)" + drops the required attr
        const char* hint        = nullptr;   // small helper text below (HTML allowed)
        const char* placeholder = nullptr;
        long        min         = 0;     // NUMBER bounds; min==max means unbounded
        long        max         = 0;
        String      value;               // current value, pre-filled into the input
    };

    // Validate-and-persist callback. `values` maps each field key to its submitted,
    // trimmed string. Return true to persist (the app writes its own NVS here) and
    // reboot; return false and set `err` to re-render the form with an error.
    using SaveFn = std::function<bool(const std::map<String,String>& values, String& err)>;

    // Serves the form at http://<mdns_host>.local (and the device IP). `title` is the
    // <h1>; `intro_html` is rendered as a banner above the form (may be empty).
    // Persists via on_save, then reboots. Never returns.
    [[noreturn]] void run(const char* mdns_host, const char* title,
                          const char* intro_html,
                          Field* fields, size_t n, SaveFn on_save);
}

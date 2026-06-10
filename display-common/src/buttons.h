#pragma once

#include <Arduino.h>

enum ButtonEvent : uint8_t {
    BTN_NONE = 0,
    BTN_D0_PRESSED,
    BTN_D1_PRESSED,
    BTN_D2_PRESSED,
};

namespace buttons {
    void begin();
    ButtonEvent poll();           // returns at most one event per call (edge-triggered, debounced)
}

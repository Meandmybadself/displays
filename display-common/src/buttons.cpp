#include "buttons.h"
#include "dd_config.h"

namespace {
    constexpr uint16_t DEBOUNCE_MS = 30;

    struct Btn {
        uint8_t  pin;
        bool     active_high;
        bool     last_raw;
        bool     stable_state;     // current debounced level
        uint32_t last_change_ms;
        bool     reported_press;   // edge already returned to caller?
    };

    Btn btns[3] = {
        { PIN_BTN_D0, false, false, false, 0, false },
        { PIN_BTN_D1, true,  false, false, 0, false },
        { PIN_BTN_D2, true,  false, false, 0, false },
    };

    bool is_pressed(const Btn& b, bool raw) {
        return b.active_high ? raw : !raw;
    }
}

namespace buttons {

void begin() {
    pinMode(PIN_BTN_D0, INPUT_PULLUP);
    pinMode(PIN_BTN_D1, INPUT_PULLDOWN);
    pinMode(PIN_BTN_D2, INPUT_PULLDOWN);
}

ButtonEvent poll() {
    uint32_t now = millis();
    static const ButtonEvent events[3] = { BTN_D0_PRESSED, BTN_D1_PRESSED, BTN_D2_PRESSED };

    for (int i = 0; i < 3; ++i) {
        Btn& b = btns[i];
        bool raw = digitalRead(b.pin);
        if (raw != b.last_raw) {
            b.last_raw = raw;
            b.last_change_ms = now;
        }
        if (now - b.last_change_ms >= DEBOUNCE_MS) {
            bool pressed = is_pressed(b, raw);
            if (pressed != b.stable_state) {
                b.stable_state = pressed;
                if (pressed && !b.reported_press) {
                    b.reported_press = true;
                    return events[i];
                }
                if (!pressed) {
                    b.reported_press = false;
                }
            }
        }
    }
    return BTN_NONE;
}

} // namespace buttons

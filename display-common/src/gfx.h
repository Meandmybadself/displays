#pragma once
//
// gfx — the shared design language: an anti-aliased text/shape blitter plus the
// standard boot/portal/status screens, drawn identically on the ST7789 (device)
// and a GFXcanvas16 (host preview, sim/).
//
// The drawing primitives are PUBLIC so each app renders its one data view in the
// same language: include this header + gfx_fonts.h, pass an AAfont* (a core face
// or one of your own) to gfx::text(), and you get the same anti-aliased Helvetica
// the rest of the firmware uses. The generic screens below never change per app.
//
#include <Arduino.h>
#include "fonts/aa_font.h"

class Adafruit_GFX;  // fwd
class IPAddress;     // fwd

namespace gfx {
#ifndef HOST_SIM
    void begin();
#else
    // Host preview (sim/): bind drawing to a caller-owned canvas, no hardware.
    void begin_sim(Adafruit_GFX* surface);
#endif
    // Raw surface, for app-specific primitives (fillRect, etc.). Valid after begin().
    Adafruit_GFX* surface();

    // --- Anti-aliased primitives (composite fg over a known solid bg) -----------
    // text(): pen origin (x,y) on the baseline, GFX cursor semantics.
    void text(const AAfont* font, int16_t x, int16_t y, const String& s,
              uint16_t fg, uint16_t bg);
    // Measure ink offset (x1,y1) and size (w,h) as Adafruit's getTextBounds would.
    void text_bounds(const AAfont* font, const String& s,
                     int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    // center_text(): horizontally centred, visual top edge at `top`.
    void center_text(const AAfont* font, int16_t top, uint16_t color, const String& s);
    // left_top(): left ink edge at `x`, visual top edge at `top`.
    void left_top(const AAfont* font, int16_t x, int16_t top, uint16_t color, const String& s);
    // Anti-aliased filled disc / ring (4x4 supersampled; for small radii).
    void disc(int16_t cx, int16_t cy, float r, uint16_t fg, uint16_t bg);
    void ring(int16_t cx, int16_t cy, float r_out, float thick, uint16_t fg, uint16_t bg);

    void clear();  // fill the whole screen with COLOR_BG

    // --- Standard design-language screens (identical across apps) ---------------
    void show_boot_message(const char* line1, const char* line2 = nullptr);
    // Heavy status screen: each word on its own line, Helvetica Neue Bold 20pt,
    // stacked into the bottom-left corner ("Loading weather", "Connecting wifi").
    void show_status(const char* text);
    void show_portal_instructions(const char* ssid, const IPAddress& ip);
    void show_factory_reset_countdown(uint8_t seconds_remaining);
    // Stage-2 onboarding screen: where to point a browser to finish setup.
    void show_setup_url(const IPAddress& ip, const char* mdns_host);
    // Small connectivity dot in the top-right corner (cyan up / amber down).
    void draw_wifi_indicator(bool wifi_ok);
}

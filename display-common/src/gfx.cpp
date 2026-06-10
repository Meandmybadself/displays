#include "gfx.h"
#include "gfx_fonts.h"
#include "dd_config.h"

#include <Adafruit_GFX.h>
#ifndef HOST_SIM
#include <Adafruit_ST7789.h>
#include <SPI.h>
#endif
#include <IPAddress.h>
#include <math.h>

namespace {
#ifndef HOST_SIM
    Adafruit_ST7789 tft_device(TFT_CS, TFT_DC, TFT_RST);
#endif
    // The drawing surface: the ST7789 on device, a GFXcanvas16 under the host
    // preview (sim/). Every helper goes through this Adafruit_GFX base pointer so
    // the identical rendering runs in both places. Bound by begin()/begin_sim().
    Adafruit_GFX* g = nullptr;

    constexpr int16_t INDICATOR_R   = 3;
    constexpr int16_t INDICATOR_PAD = 5;

    // Blend fg over bg by 4-bit coverage a (0..15), in RGB565. Reads only known
    // constants (every screen draws on COLOR_BG, elements don't overlap), so no
    // framebuffer read-back / SPI round-trip is needed.
    inline uint16_t aa_blend(uint16_t fg, uint16_t bg, uint8_t a) {
        if (a >= 15) return fg;
        uint16_t a8 = (uint16_t)a * 17;                 // 0..15 -> 0..255 (15*17=255)
        int fr = (fg >> 11) & 0x1F, fg6 = (fg >> 5) & 0x3F, fb = fg & 0x1F;
        int br = (bg >> 11) & 0x1F, bg6 = (bg >> 5) & 0x3F, bb = bg & 0x1F;
        int r = br  + (((fr  - br)  * a8 + 127) / 255);
        int gc = bg6 + (((fg6 - bg6) * a8 + 127) / 255);
        int b = bb  + (((fb  - bb)  * a8 + 127) / 255);
        return (uint16_t)((r << 11) | (gc << 5) | b);
    }
}

namespace gfx {

// --- Public primitives ------------------------------------------------------

void text_bounds(const AAfont* font, const String& s,
                 int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    int16_t minx = 32767, miny = 32767, maxx = -32768, maxy = -32768;
    int16_t cx = 0;
    for (uint16_t i = 0; i < s.length(); ++i) {
        uint8_t c = (uint8_t)s.charAt(i);
        if (c < font->first || c > font->last) continue;
        const AAglyph* gl = &font->glyph[c - font->first];
        if (gl->width && gl->height) {
            int16_t x0 = cx + gl->xOffset, y0 = gl->yOffset;
            if (x0 < minx) minx = x0;
            if (y0 < miny) miny = y0;
            if (x0 + gl->width  - 1 > maxx) maxx = x0 + gl->width  - 1;
            if (y0 + gl->height - 1 > maxy) maxy = y0 + gl->height - 1;
        }
        cx += gl->xAdvance;
    }
    if (maxx < minx) { minx = 0; miny = 0; maxx = -1; maxy = -1; }
    *x1 = minx; *y1 = miny;
    *w = (uint16_t)(maxx - minx + 1);
    *h = (uint16_t)(maxy - miny + 1);
}

void text(const AAfont* font, int16_t x, int16_t y, const String& s,
          uint16_t fg, uint16_t bg) {
    const uint8_t* bm = font->bitmap;
    g->startWrite();
    for (uint16_t i = 0; i < s.length(); ++i) {
        uint8_t c = (uint8_t)s.charAt(i);
        if (c < font->first || c > font->last) continue;
        const AAglyph* gl = &font->glyph[c - font->first];
        if (gl->width && gl->height) {
            uint32_t off = gl->bitmapOffset;
            int16_t  ox = x + gl->xOffset, oy = y + gl->yOffset;
            int      p  = 0;
            for (int16_t yy = 0; yy < gl->height; ++yy) {
                for (int16_t xx = 0; xx < gl->width; ++xx, ++p) {
                    uint8_t byte = pgm_read_byte(&bm[off + (p >> 1)]);
                    uint8_t a = (p & 1) ? (byte & 0x0F) : (byte >> 4);
                    if (a) g->writePixel(ox + xx, oy + yy, aa_blend(fg, bg, a));
                }
            }
        }
        x += gl->xAdvance;
    }
    g->endWrite();
}

void center_text(const AAfont* font, int16_t top, uint16_t color, const String& s) {
    int16_t x1, y1; uint16_t w, h;
    text_bounds(font, s, &x1, &y1, &w, &h);
    int16_t x = (TFT_WIDTH - (int16_t)w) / 2 - x1;
    text(font, x, top - y1, s, color, COLOR_BG);
}

void left_top(const AAfont* font, int16_t x, int16_t top, uint16_t color, const String& s) {
    int16_t x1, y1; uint16_t w, h;
    text_bounds(font, s, &x1, &y1, &w, &h);
    text(font, x - x1, top - y1, s, color, COLOR_BG);
}

void disc(int16_t cx, int16_t cy, float r, uint16_t fg, uint16_t bg) {
    int16_t r0 = (int16_t)ceilf(r) + 1;
    g->startWrite();
    for (int16_t dy = -r0; dy <= r0; ++dy)
        for (int16_t dx = -r0; dx <= r0; ++dx) {
            int cov = 0;
            for (int sy = 0; sy < 4; ++sy)
                for (int sx = 0; sx < 4; ++sx) {
                    float fx = dx + (sx + 0.5f) / 4.0f - 0.5f;
                    float fy = dy + (sy + 0.5f) / 4.0f - 0.5f;
                    if (fx * fx + fy * fy <= r * r) cov++;
                }
            if (cov) g->writePixel(cx + dx, cy + dy, aa_blend(fg, bg, (cov * 15 + 8) / 16));
        }
    g->endWrite();
}

void ring(int16_t cx, int16_t cy, float r_out, float thick, uint16_t fg, uint16_t bg) {
    float r_in = r_out - thick;
    int16_t r0 = (int16_t)ceilf(r_out) + 1;
    g->startWrite();
    for (int16_t dy = -r0; dy <= r0; ++dy)
        for (int16_t dx = -r0; dx <= r0; ++dx) {
            int cov = 0;
            for (int sy = 0; sy < 4; ++sy)
                for (int sx = 0; sx < 4; ++sx) {
                    float fx = dx + (sx + 0.5f) / 4.0f - 0.5f;
                    float fy = dy + (sy + 0.5f) / 4.0f - 0.5f;
                    float d2 = fx * fx + fy * fy;
                    if (d2 <= r_out * r_out && d2 >= r_in * r_in) cov++;
                }
            if (cov) g->writePixel(cx + dx, cy + dy, aa_blend(fg, bg, (cov * 15 + 8) / 16));
        }
    g->endWrite();
}

void clear() { g->fillScreen(COLOR_BG); }

Adafruit_GFX* surface() { return g; }

// --- Surface setup ----------------------------------------------------------

static void configure_surface() {
    g->fillScreen(COLOR_BG);
    g->setTextWrap(false);
    g->setTextSize(1);   // custom fonts carry their own size; never scale them
}

#ifndef HOST_SIM
void begin() {
    // Reverse TFT Feather: powering the I2C/peripheral rail also enables the display backlight rail.
    pinMode(PIN_TFT_I2C_POWER, OUTPUT);
    digitalWrite(PIN_TFT_I2C_POWER, HIGH);
    pinMode(PIN_TFT_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_TFT_BACKLIGHT, HIGH);
    delay(10);

    tft_device.init(135, 240);             // native portrait panel
    tft_device.setRotation(TFT_ROTATION);  // -> 240x135 landscape
    g = &tft_device;
    configure_surface();
}
#else
void begin_sim(Adafruit_GFX* surface) {
    g = surface;
    configure_surface();
}
#endif

// --- Standard screens -------------------------------------------------------

void show_status(const char* text_in) {
    g->fillScreen(COLOR_BG);

    constexpr int16_t MARGIN_X = 8;    // from the left edge
    constexpr int16_t MARGIN_B = 10;   // from the bottom edge (last baseline)
    const int16_t line_h = FONT_STATUS->yAdvance;

    // Split into words.
    String  s = text_in;
    String  words[6];
    uint8_t n = 0;
    int     start = 0, len = s.length();
    while (start < len && n < 6) {
        int    sp = s.indexOf(' ', start);
        String w  = (sp < 0) ? s.substring(start) : s.substring(start, sp);
        if (w.length()) words[n++] = w;
        start = (sp < 0) ? len : sp + 1;
    }

    // Stack upward from the bottom-left: the last word's baseline sits near the
    // bottom edge; earlier words climb one line at a time.
    int16_t bottom_baseline = TFT_HEIGHT - MARGIN_B;
    for (uint8_t i = 0; i < n; ++i) {
        int16_t  baseline = bottom_baseline - (int16_t)(n - 1 - i) * line_h;
        int16_t  x1, y1;
        uint16_t w, h;
        text_bounds(FONT_STATUS, words[i], &x1, &y1, &w, &h);
        text(FONT_STATUS, MARGIN_X - x1, baseline, words[i], COLOR_FG, COLOR_BG);
    }
}

void show_boot_message(const char* line1, const char* line2) {
    g->fillScreen(COLOR_BG);
    center_text(FONT_MED, TFT_HEIGHT / 2 - 20, COLOR_FG, line1);
    if (line2) {
        center_text(FONT_SMALL, TFT_HEIGHT / 2 + 10, COLOR_DIM, line2);
    }
}

void show_portal_instructions(const char* ssid, const IPAddress& ip) {
    g->fillScreen(COLOR_BG);
    center_text(FONT_MED,   8,  COLOR_ACCENT, "Setup mode");
    center_text(FONT_SMALL, 40, COLOR_FG,     "Connect phone to wifi:");
    center_text(FONT_MED,   56, COLOR_FG,     ssid);
    center_text(FONT_SMALL, 86, COLOR_DIM,    "Then open browser to:");
    center_text(FONT_SMALL, 102,COLOR_FG,     ip.toString());
    center_text(FONT_SMALL, 120,COLOR_DIM,    "(captive portal auto-pops)");
}

void show_factory_reset_countdown(uint8_t seconds_remaining) {
    g->fillScreen(COLOR_BG);
    center_text(FONT_MED, 20, COLOR_WARN, "Hold D0 to");
    center_text(FONT_MED, 48, COLOR_WARN, "reset config");
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", seconds_remaining);
    center_text(FONT_BIG, 80, COLOR_FG, buf);
}

void show_setup_url(const IPAddress& ip, const char* mdns_host) {
    g->fillScreen(COLOR_BG);
    center_text(FONT_MED,   6,  COLOR_ACCENT, "Finish setup");
    center_text(FONT_SMALL, 34, COLOR_FG,     "Open in your browser:");

    String mdns_url = String("http://") + mdns_host + ".local";
    center_text(FONT_SMALL, 54, COLOR_FG, mdns_url);

    center_text(FONT_SMALL, 72, COLOR_DIM, "or");

    String ip_url = String("http://") + ip.toString();
    center_text(FONT_SMALL, 90, COLOR_FG, ip_url);

    center_text(FONT_SMALL, 118, COLOR_DIM, "(any device on this wifi)");
}

void draw_wifi_indicator(bool wifi_ok) {
    int16_t cx = TFT_WIDTH - INDICATOR_PAD - INDICATOR_R;
    int16_t cy = INDICATOR_PAD + INDICATOR_R;
    g->fillRect(cx - INDICATOR_R - 2, cy - INDICATOR_R - 2,
                INDICATOR_R * 2 + 4, INDICATOR_R * 2 + 4, COLOR_BG);
    disc(cx, cy, (float)INDICATOR_R, wifi_ok ? COLOR_ACCENT : COLOR_WARN, COLOR_BG);
}

} // namespace gfx

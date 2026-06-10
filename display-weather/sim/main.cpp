// Host-side preview harness: render the real firmware drawing code (the shared
// gfx screens + this app's view::draw) into an in-memory GFXcanvas16 and write a
// scaled PNG — no ESP32, no flash. Built/run by scripts/preview.sh.
#include <Adafruit_GFX.h>
#include <cstring>
#include <cstdlib>
#include <vector>

#include "config.h"
#include "weather_fetcher.h"
#include "view.h"
#include <gfx.h>
#include "png.h"

static const char* arg(int argc, char** argv, const char* key, const char* def) {
    for (int i = 1; i < argc - 1; ++i)
        if (!strcmp(argv[i], key)) return argv[i + 1];
    return def;
}
static bool flag(int argc, char** argv, const char* key) {
    for (int i = 1; i < argc; ++i)
        if (!strcmp(argv[i], key)) return true;
    return false;
}

int main(int argc, char** argv) {
    const char* out   = arg(argc, argv, "--out", "preview.png");
    int         scale = atoi(arg(argc, argv, "--scale", "4"));
    if (scale < 1) scale = 1;

    GFXcanvas16 canvas(TFT_WIDTH, TFT_HEIGHT);
    gfx::begin_sim(&canvas);

    WeatherSnapshot s = {};
    s.temp_f     = (float)atof(arg(argc, argv, "--temp", "72"));
    s.high_f     = (float)atof(arg(argc, argv, "--hi",   "78"));
    s.low_f      = (float)atof(arg(argc, argv, "--lo",   "61"));
    s.pop        = (float)atof(arg(argc, argv, "--pop",  "0.2"));
    s.conditions = arg(argc, argv, "--cond", "Partly Cloudy");
    s.icon_code  = arg(argc, argv, "--icon", "02d");   // OWM icon code, e.g. 01d/10n/13d
    s.valid      = true;
    s.stale      = flag(argc, argv, "--stale");

    const char* screen = arg(argc, argv, "--screen", "weather");
    if (!strcmp(screen, "status")) {
        gfx::show_status(arg(argc, argv, "--cond", "Connecting wifi"));
    } else if (!strcmp(screen, "boot")) {
        gfx::show_boot_message("Weather Display", "starting up");
    } else if (!strcmp(screen, "portal")) {
        gfx::show_portal_instructions("WeatherDisplay-Setup", IPAddress(192, 168, 4, 1));
    } else if (!strcmp(screen, "setup")) {
        gfx::show_setup_url(IPAddress(192, 168, 1, 42), "weatherdisplay");
    } else if (!strcmp(screen, "reset")) {
        gfx::show_factory_reset_countdown(3);
    } else {
        view::draw(s);
        if (flag(argc, argv, "--wifi"))   gfx::draw_wifi_indicator(true);
        if (flag(argc, argv, "--nowifi")) gfx::draw_wifi_indicator(false);
    }

    // RGB565 framebuffer -> scaled 8-bit RGB.
    const uint16_t* fb = canvas.getBuffer();
    const int w = canvas.width(), h = canvas.height();
    const int ow = w * scale, oh = h * scale;
    std::vector<uint8_t> rgb((size_t)ow * oh * 3);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint16_t px = fb[y * w + x];
            uint8_t r = ((px >> 11) & 0x1F) * 255 / 31;
            uint8_t g = ((px >> 5)  & 0x3F) * 255 / 63;
            uint8_t b = ( px        & 0x1F) * 255 / 31;
            for (int dy = 0; dy < scale; ++dy)
                for (int dx = 0; dx < scale; ++dx) {
                    size_t o = ((size_t)(y * scale + dy) * ow + (x * scale + dx)) * 3;
                    rgb[o] = r; rgb[o + 1] = g; rgb[o + 2] = b;
                }
        }
    }

    if (!png::write_rgb(out, rgb.data(), ow, oh)) {
        fprintf(stderr, "preview: failed to write %s\n", out);
        return 1;
    }
    printf("preview: wrote %s (%dx%d)\n", out, ow, oh);
    return 0;
}

# display-common

The shared **design language and infrastructure** for the ESP32-S2 Reverse TFT
display apps ([display-weather](../display-weather), [display-stock](../display-stock),
and any future screen). Edit something here once and every consumer picks it up on
its next build.

It's a PlatformIO library. Each app references it by local path:

```ini
# <app>/platformio.ini
lib_deps =
    symlink://../display-common
    ; ... the app's own data libs (e.g. NTPClient) ...
```

## What's in the template

| Area | Module | Notes |
|------|--------|-------|
| Anti-aliased text + shapes | `gfx` (`gfx.h`/`gfx.cpp`) | 4-bit-alpha Helvetica blitter, discs/rings. The primitives are **public** so each app draws its data view in the same language. |
| Core fonts | `gfx_fonts` + `fonts/` | Helvetica Neue Bold 8/10/20/30pt. The single TU that owns the core glyph tables. |
| Loading / boot / status screens | `gfx::show_status` etc. | "heavy Helvetica, bottom-left" loading screen, boot message, factory-reset countdown. |
| Wifi setup (Stage 1) | `wifi_portal` | WiFiManager captive portal, wifi-only, Swiss-styled banner. `gfx::show_portal_instructions` on-screen. |
| App setup (Stage 2) | `setup_portal` | One Swiss-design web form, **driven by a field schema the app declares**. `gfx::show_setup_url` on-screen. |
| Geolocation | `geolocate` | WiGLE BSSID → lat/lon (used "when applicable"). |
| Buttons | `buttons` | Debounced, edge-triggered. |
| Card scripts | `scripts/` | `flash.sh`, `monitor.sh`, `_wait_for_board.sh`, `gen_fonts.sh`, `preview.sh`, `aa_fontconvert.c`. |
| Host preview | `sim/shim`, `sim/png.h` | Renders the real `gfx` code to a PNG with no hardware. |

## The seams (what stays in the app)

The library depends only on `dd_config.h` (universal pins/geometry/colours/timings).
Everything app-specific is passed in at runtime. Each app keeps:

- `src/config.h` — app constants; **must `#include <dd_config.h>`**.
- `src/storage.*` — its own NVS config struct.
- its data fetcher (`*_fetcher.*`) and `src/view.cpp` (the one data view, drawn with `gfx::` primitives + the app's own font faces).
- `src/main.cpp` — the boot state machine, wired to the shared modules.
- `scripts/app_fonts.sh` — declares the app's extra font sizes/icons (see below).
- thin `scripts/*.sh` wrappers that exec the library scripts.

## Fonts

`scripts/gen_fonts.sh` builds `aa_fontconvert`, extracts Helvetica Neue **Bold**
from `HelveticaNeue.ttc`, generates the **core** faces into this library's
`src/fonts/`, then sources the app's `scripts/app_fonts.sh` (`gen_app_fonts()`) to
generate the app's own faces into `<app>/src/fonts/`. Both the `.ttc` and the
generated `*pt7b.h` tables are gitignored (proprietary font).

Run it from an app (its wrapper passes the app dir):

```sh
cd ../display-weather && scripts/gen_fonts.sh
```

## Host preview

```sh
cd ../display-weather && scripts/preview.sh -- --screen status --cond "Connecting wifi"
```

Compiles the library `gfx` + the app's `view.cpp` + the app's `sim/main.cpp` against
a `GFXcanvas16` and writes `preview.png`.

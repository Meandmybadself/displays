# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

ESP32-S2 Arduino/PlatformIO firmware turning an Adafruit Feather ESP32-S2 Reverse TFT into a stock ticker. See `README.md` for hardware, first-boot setup, button map, and the S2 bootloader gotcha.

## Built on `display-common`

The design language and infrastructure live in a **shared sibling PlatformIO library, `../display-common`** (referenced via `lib_deps = symlink://../display-common`). Edit the library once and both this app and `display-weather` pick it up. A fresh checkout needs both repos side by side.

This app was migrated onto the template, so it now uses **anti-aliased Helvetica Neue** (the library's `gfx` blitter, not the old 1-bit GFX fonts) and a **two-stage onboarding**: Stage 1 is the wifi-only captive portal (`wifi_portal`); Stage 2 is a schema-driven web form at `stockticker.local` (`setup_portal`) that collects symbols + dwell — no longer WiFiManager custom params. The library also provides `buttons`, `wifi_portal::check_factory_reset`, the generic boot/status/portal/countdown/wifi-dot screens, `dd_config.h` (pins/geometry/colours/timeouts), the card scripts, and the host-preview shim. See `../display-common/README.md`.

This app keeps only its **app layer** in `src/`: `config.h` (Yahoo host, NTP, bounds, `COLOR_UP/DOWN`; `#include <dd_config.h>`), `storage.*` (`StockConfig` + `parse_symbols`), `stock_fetcher.*` (one-symbol-per-loop, WDT-safe), `view.cpp` (the stock screen + status-bar clock, drawn with `gfx::` primitives + app-only 15/26pt faces), and `main.cpp` (boot state machine + NTP). `scripts/*.sh` are thin wrappers that exec the library scripts; `scripts/app_fonts.sh` declares this app's 15/26pt faces.

## Commands

```sh
pio run                # build only (no board needed)
scripts/flash.sh       # wait for /dev/cu.usbmodem*, build, upload
scripts/monitor.sh     # wait for port, open serial monitor @115200
```

There is no test suite, linter, or CI — `pio run` (compile) is the only check. `scripts/flash.sh` and `scripts/monitor.sh` both block on `_wait_for_board.sh` until a USB serial port appears (override `BOARD_PORT_GLOB` / `BOARD_TIMEOUT_S`).

**Fonts must be generated before the first build.** The UI uses Helvetica Neue as Adafruit-GFX bitmap fonts, but `HelveticaNeue.ttc` and the generated `src/fonts/HelveticaNeue*pt7b.h` headers are gitignored (proprietary font). A fresh clone won't compile until you run `scripts/gen_fonts.sh` (needs `HelveticaNeue.ttc` in the repo root, `brew install freetype`, `pip3 install fonttools`, and `.pio/libdeps` populated by one `pio run`). Only `src/fonts/helvetica_neue.h` (the aggregator) is tracked. See the README "Fonts" section. To restyle, edit the `fontconvert` calls in `gen_fonts.sh` and re-run.

## Architecture

Single-threaded cooperative loop, no RTOS tasks or async. `setup()` boots (buttons → display → storage → optional factory-reset hold → load config or launch captive portal → wifi connect → fetcher/NTP init), then `loop()` runs everything by polling `millis()` deltas.

**The dominant constraint: nothing in `loop()` may block past the ESP32 loopTask watchdog (~5s).** This is *why* the code is shaped the way it is:
- `stock_fetcher::refresh_next()` fetches exactly **one symbol per loop iteration** and returns `true` only when a full round of the symbol list completes. Never refactor this into a synchronous "fetch all symbols" call — it would trip the WDT.
- `HTTP_TIMEOUT_MS` (4000ms in `config.h`) is deliberately under the WDT. Keep it there.
- `main.cpp` drives a small refresh state machine via flags (`refresh_active`, `have_data`) and timestamps (`last_refresh_ms`, `last_fetch_step_ms`, etc.), paced by `FETCH_STEP_GAP_MS`.

`run_portal()` and the wifi-failure paths call `ESP.restart()` and never return (marked `[[noreturn]]`) — config changes always take effect via reboot, not live re-read.

### Modules (`src/`, each a `namespace` over a `.h`/`.cpp` pair)
- `main.cpp` — state machine + `loop()`; the only place modules are wired together.
- `config.h` — single source of truth for pins, timings, bounds, colors (RGB565), Yahoo host/path, NTP. **Tune behavior here**, not in scattered literals.
- `storage` — NVS-backed config (symbols CSV + dwell seconds); also owns CSV→array parsing (`parse_symbols`).
- `wifi_setup` — WiFiManager wrapper: captive portal (with custom symbol/dwell params), `connect()` (reuses esp32 wifi NVS creds), `factory_reset()`.
- `stock_fetcher` — Yahoo `v8/finance/chart` client; owns the `StockQuote` cache, round-robin refresh, valid/stale tracking, and prev/next-valid index navigation.
- `display` — all ST7789 drawing (boot/portal messages, stock view, status bar, no-data, reset countdown).
- `buttons` — debounced, edge-triggered events; `poll()` returns one `ButtonEvent` per call.

### Key conventions
- Config flows one direction: portal/NVS → `StockConfig` (loaded once in `setup()`) → modules. Changing symbols/dwell requires a reboot.
- A `StockQuote` becomes `valid` after its first successful fetch and `stale` after `STALE_THRESHOLD_MS`; the display distinguishes never-fetched from stale.
- TLS uses `setInsecure()` (no cert validation) and the Yahoo endpoint is unofficial — both load-bearing assumptions, see README Caveats.

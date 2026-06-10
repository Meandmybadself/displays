# displays

A family of small ESP32-S2 desk displays for the
[Adafruit Feather ESP32-S2 Reverse TFT](https://www.adafruit.com/product/5345)
(240×135 ST7789), sharing one design language.

| Project | What it shows |
|---------|---------------|
| [`display-common`](display-common/) | The shared template — design language + infrastructure, consumed by the apps below. |
| [`display-weather`](display-weather/) | Current weather (temp, conditions, today's high/low, precip) from OpenWeatherMap. |
| [`display-stock`](display-stock/) | A rotating stock ticker (price + % change) from Yahoo Finance. |

## The shared template

[`display-common`](display-common/) is a local-path PlatformIO library that every
app pulls in with:

```ini
lib_deps = symlink://../display-common
```

so the projects must stay **side by side** in this repo. Change the template once
and both apps pick it up on their next build. It owns:

- the anti-aliased Helvetica Neue `gfx` screen toolkit (blitter + all generic
  boot / loading / captive-portal / setup / countdown / wifi-dot screens),
- two-stage wifi onboarding — `wifi_portal` (captive portal) → `setup_portal`
  (a schema-driven local web form),
- WiGLE geolocation, debounced buttons, the shared hardware constants,
- the card scripts (flash / monitor / detect, font generation, host preview).

See [`display-common/README.md`](display-common/README.md) for the architecture
and the seams between the library and each app.

## Build

Each app is a standalone PlatformIO project. From an app directory:

```sh
scripts/gen_fonts.sh    # one-time: generate the (gitignored) Helvetica glyph tables
pio run                 # compile check
scripts/flash.sh        # build + upload to a connected Feather
scripts/preview.sh -- --screen weather   # render a screen to a PNG on the host
```

## Fonts

The UI uses Helvetica Neue. `HelveticaNeue.ttc` (proprietary) and the generated
`HelveticaNeue*pt7b.h` glyph tables are **gitignored** — run `scripts/gen_fonts.sh`
after cloning. The Weather Icons font (Erik Flowers, SIL OFL 1.1) and its generated
tables are checked in.

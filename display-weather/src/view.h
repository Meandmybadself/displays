#pragma once

struct WeatherSnapshot;  // fwd

namespace view {
    // The weather data screen: big left-justified temperature with a geometric
    // degree ring, and a right column with the condition icon, low/high, and precip.
    // Drawn with the shared gfx primitives + this app's own font faces.
    void draw(const WeatherSnapshot& s);
}

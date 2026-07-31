#include "Adafruit_ST7789.h"

// The simulated panel. Separate from the GFXcanvas16 the firmware draws into:
// the firmware composites into the canvas, then flushes dirty rectangles here.
// Keeping them distinct is what lets the simulator reproduce the dirty-rect
// mismatch described in the audit as M13 -- anything never flushed simply
// never appears, exactly as on hardware.
static uint16_t g_panel[240 * 320];

const uint16_t* Adafruit_ST7789::panelBuffer() { return g_panel; }

void Adafruit_ST7789::panelClear(uint16_t color) {
    for (size_t i = 0; i < sizeof(g_panel) / sizeof(g_panel[0]); ++i) g_panel[i] = color;
}

void Adafruit_ST7789::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || y < 0 || x >= 240 || y >= 320) return;
    g_panel[y * 240 + x] = color;
}

void Adafruit_ST7789::fillScreen(uint16_t color) { panelClear(color); }

void Adafruit_ST7789::writePixels(uint16_t* colors, uint32_t len) {
    for (uint32_t i = 0; i < len; ++i) {
        const int px = _wx + _wcol;
        const int py = _wy + _wrow;
        if (px >= 0 && py >= 0 && px < 240 && py < 320) {
            g_panel[py * 240 + px] = colors[i];
        }
        if (++_wcol >= _ww) { _wcol = 0; if (++_wrow >= _wh) _wrow = 0; }
    }
}

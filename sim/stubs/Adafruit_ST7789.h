// The ST7789 panel driver. In the simulator the canvas is the source of truth
// and the "panel" is whatever frontend blits it, so these are no-ops -- but the
// colour constants are real, because widget code uses them everywhere.
#ifndef ADAFRUIT_ST7789_H_STUB
#define ADAFRUIT_ST7789_H_STUB

#include "Arduino.h"
#include "Adafruit_GFX.h"
#include "SPI.h"

#define ST77XX_BLACK   0x0000
#define ST77XX_WHITE   0xFFFF
#define ST77XX_RED     0xF800
#define ST77XX_GREEN   0x07E0
#define ST77XX_BLUE    0x001F
#define ST77XX_CYAN    0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW  0xFFE0
#define ST77XX_ORANGE  0xFC00

class Adafruit_ST7789 : public Adafruit_GFX {
public:
    Adafruit_ST7789(int8_t cs, int8_t dc, int8_t rst) : Adafruit_GFX(240, 320) {
        (void)cs; (void)dc; (void)rst;
    }
    void init(uint16_t w, uint16_t h, uint8_t mode = 0) { (void)w; (void)h; (void)mode; }
    void setSPISpeed(uint32_t) {}
    void invertDisplay(bool) {}
    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
        _wx = x; _wy = y; _ww = w; _wh = h; _wcol = 0; _wrow = 0;
    }
    void startWrite() {}
    void endWrite() {}

    // Records what the firmware pushes, so the frontend can honour dirty
    // rectangles exactly as the panel would -- if a widget forgets to mark
    // itself dirty, the simulator shows the same stale pixels the device does.
    void writePixels(uint16_t* colors, uint32_t len);

    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void fillScreen(uint16_t color) override;

    /** RGB565 framebuffer as the panel currently holds it. */
    static const uint16_t* panelBuffer();
    static void panelClear(uint16_t color = 0);

private:
    uint16_t _wx = 0, _wy = 0, _ww = 0, _wh = 0, _wcol = 0, _wrow = 0;
};

#endif

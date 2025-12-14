#include <Wire.h>
#include "Display.hpp"
#include <vector>

  #define TFT_CS         D0
  #define TFT_RST        -1
  #define TFT_DC         D2

Adafruit_ST7789 Display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);

std::vector<DirtyRect> dirtyRects;

namespace Disp {
    void init() {
        Display.init(240,320,SPI_MODE3);
        Display.setSPISpeed(48000000);     // faster if stable
        Display.fillScreen(ST77XX_BLACK);
        Display.invertDisplay(false);
        clear();
    }

    void clear(uint16_t color) {
        canvas.fillScreen(color);
    }

    void markDirty(int16_t x, int16_t y, int16_t w, int16_t h) {
        dirtyRects.push_back({x, y, w, h});
    }

    void resetDirty() {
        dirtyRects.clear();
    }

    void flush() {
        for (uint8_t i = 0; i < dirtyRects.size(); i++) {
            auto& r = dirtyRects[i];

            int16_t x = max(0,r.x);
            int16_t y = max(0,r.y);
            int16_t w = min(r.w, SCREEN_WIDTH-x);
            int16_t h = min(r.h, SCREEN_HEIGHT-y);

            Display.startWrite();
            Display.setAddrWindow(x, y, w, h);

            uint16_t* buf = canvas.getBuffer();
            buf += (r.y * 240) + r.x;

            uint32_t rows = r.h;
            uint32_t cols = r.w;

            for (uint32_t y = 0; y < rows; y++) {
                Display.writePixels(buf, cols);
                buf += 240; // skip to next full row
            }

            Display.endWrite();
        }

        dirtyRects.clear(); // reset the list after every frame
    }

    void setTextSize(uint8_t s) { canvas.setTextSize(s); }
    void setCursor(int x, int y) { canvas.setCursor(x, y); }

    void setTextColor(uint16_t c) { canvas.setTextColor(c,!c); }
    void setTextColor(uint16_t c, uint16_t bg) { canvas.setTextColor(c, bg); }

    void text(int x, int y, const String& s, bool invert) {
        if (invert) {
            canvas.fillRect(0, y - 1, SCREEN_WIDTH, 10, ST77XX_WHITE);
            canvas.setTextColor(ST77XX_BLACK);
        } else {
            canvas.setTextColor(ST77XX_WHITE);
        }
        canvas.setCursor(x, y);
        canvas.print(s);
    }

    void drawText(int x, int y, const String &s, uint16_t color) {
        canvas.setTextColor(color);
        canvas.setCursor(x, y);
        canvas.print(s);
    }

    void drawTextInverted(int x, int y, const String& s) {
        int16_t x1, y1;
        uint16_t w, h;
        canvas.getTextBounds(s, x, y, &x1, &y1, &w, &h);
        canvas.fillRect(x1, y1, w, h, ST77XX_WHITE);
        canvas.setTextColor(ST77XX_BLACK);
        canvas.setCursor(x, y);
        canvas.print(s);
    }

    void fillRect(int x, int y, int w, int h, uint16_t color) {
        canvas.fillRect(x, y, w, h, color);
    }

    void drawRect(int x, int y, int w, int h, uint16_t color) {
        canvas.drawRect(x, y, w, h, color);
    }

    void getTextBounds(const String text, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        canvas.getTextBounds(text, x, y, x1, y1, w, h);
    }

    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color) {
        canvas.drawBitmap(x,y,bitmap,w,h,color);
    }

    int16_t getCursorX() {
        return canvas.getCursorX();
    }

    int16_t getCursorY() {
        return canvas.getCursorY();
    }

    void printFloat(float data, int precision) {canvas.print(data, precision);};
}
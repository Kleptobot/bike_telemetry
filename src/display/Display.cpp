#include <Wire.h>
#include "Display.hpp"

  #define TFT_CS         4
  #define TFT_RST        16
  #define TFT_DC         5

Adafruit_ST7789 Display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

static inline uint16_t toAdafruitColor(DispCol c) {
  // Adafruit SSD1306 uses WHITE and BLACK macros; map our enum to those
  return (c == DispCol::WHITE) ? ST77XX_WHITE : ST77XX_BLACK;
}

namespace Disp {
    void init() {
        Display.init(240,320);
        Display.fillScreen(ST77XX_BLACK);
    }

    void clear() {
        Display.fillScreen(ST77XX_BLACK);
        Display.setTextSize(1);
        Display.setTextColor(ST77XX_WHITE);
    }

    void text(int x, int y, const String& s, bool invert) {
        if (invert) {
            Display.fillRect(0, y - 1, SCREEN_WIDTH, 10, ST77XX_WHITE);
            Display.setTextColor(ST77XX_BLACK);
        } else {
            Display.setTextColor(ST77XX_WHITE);
        }
        Display.setCursor(x, y);
        Display.print(s);
    }

    void flush() { Display.fillScreen(ST77XX_BLACK); }

    void setTextSize(uint8_t s) {
    Display.setTextSize(s);
    }

    void drawText(int x, int y, const String &s, DispCol color) {
        Display.setTextColor(toAdafruitColor(color));
        Display.setCursor(x, y);
        Display.print(s);
    }

    void fillRect(int x, int y, int w, int h, DispCol color) {
        Display.fillRect(x, y, w, h, toAdafruitColor(color));
    }

    void drawRect(int x, int y, int w, int h, DispCol color) {
        Display.drawRect(x, y, w, h, toAdafruitColor(color));
    }

    void setCursor(int x, int y) {
        Display.setCursor(x, y);
    }

    void getTextBounds(const String text, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        Display.getTextBounds(text, x, y, x1, y1, w, h);
    }

    void setTextColor(DispCol color) {
        Display.setTextColor(toAdafruitColor(color));
    }

    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, DispCol color) {
        Display.drawBitmap(x,y,bitmap,w,h,toAdafruitColor(color));
    }

    int16_t getCursorX() {
        return Display.getCursorX();
    }

    int16_t getCursorY() {
        return Display.getCursorY();
    }
}
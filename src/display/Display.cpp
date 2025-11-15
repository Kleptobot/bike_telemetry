#include <Wire.h>
#include "Display.hpp"

  #define TFT_CS         D0
  #define TFT_RST        -1
  #define TFT_DC         D2

Adafruit_ST7789 Display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

namespace Disp {
    void init() {
        Display.init(240,320);
        Display.setSPISpeed(40000000);
        Display.fillScreen(ST77XX_BLACK);
        Display.invertDisplay(false);
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

    void drawText(int x, int y, const String &s, uint16_t color) {
        Display.setTextColor(color);
        Display.setCursor(x, y);
        Display.print(s);
    }

    void fillRect(int x, int y, int w, int h, uint16_t color) {
        Display.fillRect(x, y, w, h, color);
    }

    void drawRect(int x, int y, int w, int h, uint16_t color) {
        Display.drawRect(x, y, w, h, color);
    }

    void setCursor(int x, int y) {
        Display.setCursor(x, y);
    }

    void getTextBounds(const String text, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        Display.getTextBounds(text, x, y, x1, y1, w, h);
    }

    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {
        Display.drawBitmap(x,y,bitmap,w,h,color);
    }

    int16_t getCursorX() {
        return Display.getCursorX();
    }

    int16_t getCursorY() {
        return Display.getCursorY();
    }

    void printFloat(float data, int precision) {Display.print(data, precision);};
}
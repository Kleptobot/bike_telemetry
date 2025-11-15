#pragma once
#include <vector>
#include <stdint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

extern Adafruit_ST7789 Display;
extern GFXcanvas16 canvas;

struct DirtyRect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

namespace Disp {

    void init();                
    void clear(uint16_t color = ST77XX_BLACK);
    void markDirty(int16_t x, int16_t y, int16_t w, int16_t h);
    void resetDirty();
    void flush();

    void setTextSize(uint8_t s);
    void setCursor(int x, int y);
    void setTextColor(uint16_t color);
    void setTextColor(uint16_t color, uint16_t background);

    void drawText(int x, int y, const String& s, uint16_t color = ST77XX_WHITE);
    void drawTextInverted(int x, int y, const String& s);

    void fillRect(int x, int y, int w, int h, uint16_t color);
    void drawRect(int x, int y, int w, int h, uint16_t color);

    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);

    template<typename T>
    inline void print(const T& data) { canvas.print(data); }

    void printFloat(float data, int precision);
    void getTextBounds(const String text, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
}
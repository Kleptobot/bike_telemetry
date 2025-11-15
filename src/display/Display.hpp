#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

extern Adafruit_ST7789 Display;

namespace Disp {
  void init();                // initialize hardware
  void clear();               // clear screen & setup text params
  void flush();               // display.display()

  void setTextSize(uint8_t s = 1);
  void drawText(int x, int y, const String &s, uint16_t color = ST77XX_WHITE);
  void fillRect(int x, int y, int w, int h, uint16_t color);
  void drawRect(int x, int y, int w, int h, uint16_t color);
  void setCursor(int x, int y);
  int16_t getCursorX();
  int16_t getCursorY();
  void getTextBounds(const String text, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
  inline void setTextColor(uint16_t color) { Display.setTextColor(color,!color);}
  inline void setTextColor(uint16_t color, uint16_t background) { Display.setTextColor(color,background);}
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
  template <typename T> void print(T data) {Display.print(data);};
  void printFloat(float data, int precision);
}
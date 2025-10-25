#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

extern Adafruit_ST7789 Display;

// A lightweight colour enum so UI code stays independent of Adafruit macros.
enum class DispCol : uint8_t {
  BLACK = 0,
  WHITE = 1
};

namespace Disp {
  void init();                // initialize hardware
  void clear();               // clear screen & setup text params
  void flush();               // display.display()

  void setTextSize(uint8_t s = 1);
  void drawText(int x, int y, const String &s, DispCol color = DispCol::WHITE);
  void fillRect(int x, int y, int w, int h, DispCol color);
  void drawRect(int x, int y, int w, int h, DispCol color);
  void setCursor(int x, int y);
  int16_t getCursorX();
  int16_t getCursorY();
  void getTextBounds(const String text, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
  void setTextColor(DispCol color);
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, DispCol color);
  template <typename T> void print(T data) {Display.print(data);};
}
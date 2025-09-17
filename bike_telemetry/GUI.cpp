#include "GUI.h"


Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SettingsItem SettingsItems[s16NumSettings] = {
  {epd_bitmap_clock_large,32,32,"Time"},
  {epd_bitmap_bluetooth_large,32,32,"BLE"},
  {epd_bitmap_heart_large,32,32,"Info"},
  {epd_bitmap_left_arrow_large,32,32,"Back"},
  {epd_bitmap_antenna_large,32,32,"GPS"}
};

void drawSettingsmenuItem(int x, int y, bool focus, int16_t menuIndex) {
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(x, y);
 

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(SettingsItems[menuIndex].name, x, y, &x1, &y1, &w, &h);
  
  display.drawBitmap(x+2, y+1, SettingsItems[menuIndex].img, SettingsItems[menuIndex].x, SettingsItems[menuIndex].y, 1);
  display.setCursor(x+32+8, y+8);
  display.print(SettingsItems[menuIndex].name);
  if (focus)
    display.drawRect(x, y, 127, 34, 1);
}

void drawSettings(int16_t s16SettingsSel){

  display.setTextColor(SH110X_WHITE);

  int16_t aboveIndex = s16SettingsSel-1;
  if(s16SettingsSel == 0)
    aboveIndex = s16NumSettings-1;

  int16_t belowIndex = s16SettingsSel+1;
  if(s16SettingsSel == (s16NumSettings-1))
    belowIndex = 0;

  drawSettingsmenuItem(0, 13, false, aboveIndex);
  drawSettingsmenuItem(0, 47, true, s16SettingsSel);
  drawSettingsmenuItem(0, 81, false, belowIndex);

}

void drawMenuStopped(int x, int y) {
  display.drawBitmap(x - 32, y - 8, epd_bitmap_gear , 16, 16, 1);
  display.drawBitmap(x - 8, y - 8, epd_bitmap_play, 16, 16, 1);
  display.drawBitmap(x + 16, y -8, epd_bitmap_power, 16, 16, 1);

}

void drawMenuRunning(int x, int y) {
  //display.drawBitmap( x-8, y-26, epd_bitmap_UP,16, 16, 1);
  //display.drawBitmap( x-32, y-8, epd_bitmap_loop,16, 16, 1);
  // if (paused) {
  //   display.drawBitmap(x - 8, y - 8, epd_bitmap_play, 16, 16, 1);
  // } else {
  //   display.drawBitmap(x - 8, y - 8, epd_bitmap_pause, 16, 16, 1);
  // }
  display.drawBitmap(x - 8, y - 8, epd_bitmap_loop, 16, 16, 1);
  display.drawBitmap(x + 16, y - 8, epd_bitmap_stop, 16, 16, 1);
}

void drawSelectable(int16_t x, int16_t y, const uint8_t* bitmap, const char* text, bool focus, bool selected) {
  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(2);
  display.getTextBounds(text, x + 16, y, &x1, &y1, &w, &h);
  if (selected) {
    display.setTextColor(SH110X_BLACK);
    display.fillRect(x - 1, y - 1, w + 17, h + 1, 1);
    display.setCursor(x + 16, y);
    display.drawBitmap(x, y, bitmap, 16, 16, 1);
    display.print(text);
  } else {
    display.setTextColor(SH110X_WHITE);
    if (focus)
      display.drawRect(x - 2, y - 2, w + 18, h + 2, 1);
    display.setCursor(x + 16, y);
    display.drawBitmap(x, y, bitmap, 16, 16, 1);
    display.print(text);
  }
  display.setTextColor(SH110X_WHITE);
}

void drawSelectable(int16_t x, int16_t y, const char* text, bool focus, bool selected) {
  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(2);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (selected) {
    display.setTextColor(SH110X_BLACK);
    display.fillRect(x - 1, y - 1, w, h, 1);
    display.setCursor(x, y);
    display.print(text);
  } else {
    display.setTextColor(SH110X_WHITE);
    if (focus)
      display.drawRect(x - 2, y - 2, w + 2, h + 2, 1);
    display.setCursor(x, y);
    display.print(text);
  }
  display.setTextColor(SH110X_WHITE);
}

void drawSelectable(int16_t x, int16_t y, int num, bool focus, bool selected) {
  int16_t x1, y1;
  uint16_t w, h;

  String text;

  if (num < 10)
    text = "0" + String(num);
  else if (num > 99)
    text = "99";
  else
    text = String(num);

  display.setTextSize(2);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (selected) {
    display.setTextColor(SH110X_BLACK);
    display.fillRect(x - 1, y - 1, w, h, 1);
    display.setCursor(x, y);
    display.print(text);
  } else {
    display.setTextColor(SH110X_WHITE);
    if (focus)
      display.drawRect(x - 2, y - 2, w + 2, h + 2, 1);
    display.setCursor(x, y);
    display.print(text);
  }
  display.setTextColor(SH110X_WHITE);
}

void drawSelectable(int16_t x, int16_t y, const char* text, int num, bool focus, bool selected) {
  int16_t x1, y1;
  uint16_t w, h;
  String numText;

  numText = String(num);
  display.getTextBounds(numText, x, y, &x1, &y1, &w, &h);

  display.setTextSize(1);  
  if (selected) {
    display.setTextColor(SH110X_BLACK);
    display.fillRect(0, y - 1, 127, 17, 1);
    display.setCursor(2, y);
    display.print(text);
    display.setCursor(128-w-2, y+8);
    display.print(numText);
  } else {
    display.setTextColor(SH110X_WHITE);
    if (focus)
      display.drawRect(0, y - 2, 128, 19, 1);
    display.setCursor(2, y);
    display.print(text);
    display.setCursor(128-w-2, y+8);
    display.print(numText);
  }
  display.setTextColor(SH110X_WHITE);
}
#ifndef _GUI_H_
#define _GUI_H_

#include <Arduino.h>
//Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Adafruit_SH110X.h>
#include <RTClib.h>

#include "UI/GFX.h"

#define i2c_Address 0x3c   //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_HEIGHT 128  // OLED display height, in pixels
#define TFT_CS D0
#define TFT_RST -1
//#define TFT_DC        D2
#define OLED_RESET -1  //   QT-PY / XIAO
extern Adafruit_SH1107 display;

#define s16NumSettings 5

struct SettingsItem {
    const unsigned char* img;
    int x;
    int y;
    char name[32];
};

extern SettingsItem SettingsItems[s16NumSettings];

void drawSettingsmenuItem(int x, int y, bool focus, int16_t menuIndex);

void drawSettings(int16_t s16SettingsSel);

/**
 * Used to draw the menu cluster for when no logging is occuring
 * @param x x coordinate of the center of the cluster
 * @param y y coordinate of the center of the cluster
 */
void drawMenuStopped(int x, int y);

/**
 * Used to draw the menu cluster for when logging is running
 * @param x x coordinate of the center of the cluster
 * @param y y coordinate of the center of the cluster
 */
void drawMenuRunning(int x, int y);

/**
 * Used to draw a graphical component that consists of an image and text
 * the component provides focus feedback by showing a border and selected
 * feedback by inverting the background and foreground colours
 * @param x x coordinate of the center of the component
 * @param y y coordinate of the center of the component
 * @param bitmap bitap image to display, should be 16x16
 * @param text text to display
 * @param focus if true the component will display a border
 * @param selected if true the component will have a solid background with items rendered as black
 */
void drawSelectable(int16_t x, int16_t y, const uint8_t* bitmap, const char* text, bool focus, bool selected);

/**
 * Used to draw a graphical component that consists of text
 * the component provides focus feedback by showing a border and selected
 * feedback by inverting the background and foreground colours
 * @param x x coordinate of the center of the component
 * @param y y coordinate of the center of the component
 * @param text text to display
 * @param focus if true the component will display a border
 * @param selected if true the component will have a solid background with items rendered as black
 */
void drawSelectable(int16_t x, int16_t y, const char* text, bool focus, bool selected);

/**
 * Used to draw a graphical component that consists of an integer
 * the component provides focus feedback by showing a border and selected
 * feedback by inverting the background and foreground colours
 * @param x x coordinate of the center of the component
 * @param y y coordinate of the center of the component
 * @param num integer to display
 * @param focus if true the component will display a border
 * @param selected if true the component will have a solid background with items rendered as black
 */
void drawSelectable(int16_t x, int16_t y, int num, bool focus, bool selected);

/**
 * Used to draw a graphical component that consists of text and an integer
 * the component provides focus feedback by showing a border and selected
 * feedback by inverting the background and foreground colours
 * @param x x coordinate of the center of the component
 * @param y y coordinate of the center of the component
 * @param text text to display
 * @param num integer to display
 * @param focus if true the component will display a border
 * @param selected if true the component will have a solid background with items rendered as black
 */
void drawSelectable(int16_t x, int16_t y, const char* text, int num, bool focus, bool selected);

void drawDate(int x, int y, DateTime now, int focus, bool select);

void drawTime(int x, int y, DateTime now, int focus, bool select);

#endif // _GUI_H_
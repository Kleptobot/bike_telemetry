#ifndef GUI_H
#define GUI_H

#include <Adafruit_SH110X.h>
#include "GFX.h"

#define nHeldPressTime 1500
#define nShortPressTime 400

#define i2c_Address 0x3c   //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_HEIGHT 128  // OLED display height, in pixels
#define TFT_CS D0
#define TFT_RST -1
//#define TFT_DC        D2
#define OLED_RESET -1  //   QT-PY / XIAO
//Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

class button
{
  private:
    bool _state, _state_prev;
    bool _RE, _FE;
    bool _long, _short, _seen, _short_held;
    uint16_t _mult;
    uint64_t _pressTime;

  public:

    void process(bool state, uint64_t current_time)
    {
      _state = state;
      _RE = _state && !_state_prev;
      _FE = !_state && _state_prev;
      if (_RE)
        _pressTime = current_time;
      _long = _state && ((current_time - _pressTime) > nHeldPressTime);
      _mult = (current_time - _pressTime) / nHeldPressTime;
      if(_short)
        _short=false;
      _short = _FE && ((current_time - _pressTime) < nShortPressTime) && _seen;
      _short_held = _state && !_long && ((current_time - _pressTime) > nShortPressTime);
      if(!_state){
        _seen = false;
        _mult = 0;
      }
      _state_prev = _state;
    };

    bool state(){return _state;};
    bool RE(){return _RE;};
    bool FE(){return _FE;};
    bool shortPress(){return _short;};
    bool shortHeld(){return _short_held;};
    bool longHeld(){return _long;};
    uint16_t mult(){return _mult;};
};

struct menu_item {
  bool focus;
  bool select;
};

class menu{
  private:
    std::vector<std::vector<menu_item*>> _items;
    uint16_t _vert, _hoz;
    uint16_t _max_hoz;
    uint32_t _index;

    void updateSafes(){
      _vert = (uint8_t)((_index & 0x00FF0000)>>16);
      _hoz = (uint8_t)((_index & 0x0000FF00)>>8);
    }

    void checkHoz(){

      if(_hoz >= _items[_vert].size()-1){
        _index = _index & 0xFFFF00FF; //need to zero hoz field so the bits get set correctly on the next line
        _index = _index | ((_items[_vert].size()-1)<<8);
      }

      updateSafes();
    }

    void process(){
      uint8_t sel = (uint8_t)(_index & 0x000000FF);

      for(int i=0; i<_items.size(); i++){
        for(int j=0; j<_items[i].size(); j++){
          if(i==_vert && j==_hoz){
            _items[i][j]->focus = true;
            _items[i][j]->select= sel==1;
          }
        }
      }
    }

  
  public:
    menu(std::vector<std::vector<menu_item*>> items){
      _items = items;
    }

    menu_item* operator()(int, int);

    void incVert(){
      if(_index && 0x00000001 == 0){
        if(_vert < _items.size()-1){
          _index += 0x00010000;
        }else{
          _index = _index & 0xFF00FFFF;
        }
      }

      checkHoz();
      process();
    }

    void decVert(){
      if(_index & 0x00000001 == 0){
        if(_vert >0){
          _index -= 0x00010000;
        }else{
          _index = _index & 0xFF00FFFF; //this line is probabaly unesecary because we have already checked that this range is zero
          _index = _index | ((_items.size()-1)<<16);
        }
      }

      checkHoz();
      process();
    }

    void incHoz(){
      if(_index & 0x00000001 == 0){
        if(_hoz < _items[_vert].size()-1){
          _index += 0x00000100;
        }else{
          _index = _index & 0xFFFF00FF;
        }
      }

      updateSafes();
      process();
    }

    void decHoz(){
      if(_index & 0x00000001 == 0){
        if(_hoz > 0){
          _index -= 0x00000100;
        }else{
          _index = _index & 0xFFFF00FF; //this line is probabaly unesecary because we have already checked that this range is zero
          _index = _index | ((_items[_vert].size()-1)<<8);
        }
      }

      updateSafes();
      process();
    }

    void toggleSel(){_index = _index ^ 1;}

    bool selected(){return (uint8_t)(_index & 0x000000FF) == 1;}

    void reset(){_index=0; process();}

    uint8_t vert(){return _vert;}
    uint8_t hoz(){return _hoz;}

    uint8_t vertLen(){return _items.size();}

};

menu_item* menu::operator()(int vert, int hoz)
{
  uint16_t _vert, _hoz;

  _vert = vert;
  if(_vert>=_items.size())
    _vert=_items.size()-1;

  _hoz=hoz;
  if(_hoz>=_items[_vert].size())
    _hoz = _items[_vert].size()-1;
  

  return _items[_vert][_hoz];
}

button UP, DOWN, LEFT, RIGHT, CENTER;

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

/**
 * Used to draw the menu cluster for when no logging is occuring
 * @param x x coordinate of the center of the cluster
 * @param y y coordinate of the center of the cluster
 */
void drawMenuStopped(int x, int y) {
  display.drawBitmap(x - 32, y - 8, epd_bitmap_gear , 16, 16, 1);
  display.drawBitmap(x - 8, y - 8, epd_bitmap_play, 16, 16, 1);
  display.drawBitmap(x + 16, y -8, epd_bitmap_power, 16, 16, 1);
}

#endif
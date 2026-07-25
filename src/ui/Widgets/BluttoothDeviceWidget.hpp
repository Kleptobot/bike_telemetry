#pragma once
#include "display/Display.hpp"
#include "UI/Widgets/Widget.hpp"
#include "HAL/BluetoothInterface.hpp"
#include "UI/GFX.h"
#include "UIEventBus.hpp"

class BluetoothDeviceWidget :  public Widget {
public:
  BluetoothDeviceWidget(int x, int y, BluetoothDevice& device) : 
    Widget(x,y,128,30),
    _device(device) {visible = false;}

  void update(float dt) override {
  }
        
  void invalidate() override {
      Disp::markDirty(_x-2, _y-2, width()+4, height()+4);
  }

  void render() override {
    if (!visible) {
        Disp::fillRect(_x,_y,_width,_height,ST77XX_BLACK);
        return;
    } 

    Disp::setTextSize(1);
    Disp::setCursor(_x, _y);
    String tempString = _device.name;

    int16_t x1, y1;
    uint16_t w, h;

    Disp::getTextBounds(tempString, _x, _y, &x1, &y1, &w, &h);

    Disp::setTextColor(ST77XX_WHITE);
    if (focused)
        Disp::drawRect(_x - 2, _y - 2, 127, 30, ST77XX_WHITE);
    Disp::setCursor(_x, _y);
    Disp::print(tempString);
    Disp::drawBitmap(_x, _y + 10, epd_bitmap_down_right, 16, 16, ST77XX_WHITE);

    if (_device.saved) {
      Disp::drawBitmap(_x + 18, _y + 10, epd_bitmap_save, 16, 16, ST77XX_WHITE);

      if (_device.connected)
          Disp::drawBitmap(_x + 34, _y + 10, epd_bitmap_Bluetooth, 16, 16, ST77XX_WHITE);

      Disp::drawBitmap(_x + 50, _y + 10, epd_bitmap_battery, 32, 16, ST77XX_WHITE);
      Disp::setCursor(_x + 57, _y + 15);
    }
    Disp::print(_device.batt);
  }

  void render(int x, int y) override {
    bool _invalidate = (x != this->_x) || (y != this->_y);
    if(_invalidate) invalidate();
    this->_x = x;
    this->_y = y;
    if(_invalidate) invalidate();
    render();
  }

private:
  BluetoothDevice& _device;
};

#pragma once
#include "display/Display.hpp"
#include "UI/Widgets/Widget.hpp"
#include "HAL/BluetoothInterface.hpp"
#include "UI/GFX.h"
#include "UIEventBus.hpp"
#include "DataModel/DataModel.hpp"

class BluetoothDeviceWidget :  public Widget {
public:
  BluetoothDeviceWidget(int x, int y, BluetoothDevice& device) : 
    Widget(x,y,128,30),
    _device(device) {visible = false;}

  void update(float dt) override {
  }
        
  void invalidate() override {
      Disp::markDirty(x-2, y-2, width()+4, height()+4);
  }

  void render() override {
    if (!visible) {
        Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
        return;
    } 

    Disp::setTextSize(1);
    Disp::setCursor(x, y);
    String tempString = _device.name;

    int16_t x1, y1;
    uint16_t w, h;

    Disp::getTextBounds(tempString, x, y, &x1, &y1, &w, &h);

    Disp::setTextColor(ST77XX_WHITE);
    if (focused)
        Disp::drawRect(x - 2, y - 2, 127, 30, ST77XX_WHITE);
    Disp::setCursor(x, y);
    Disp::print(tempString);
    Disp::drawBitmap(x, y + 10, epd_bitmap_down_right, 16, 16, ST77XX_WHITE);

    if (_device.saved) {
      Disp::drawBitmap(x + 18, y + 10, epd_bitmap_save, 16, 16, ST77XX_WHITE);

      if (_device.connected)
          Disp::drawBitmap(x + 34, y + 10, epd_bitmap_Bluetooth, 16, 16, ST77XX_WHITE);

      Disp::drawBitmap(x + 50, y + 10, epd_bitmap_battery, 32, 16, ST77XX_WHITE);
      Disp::setCursor(x + 57, y + 15);
    }
    Disp::print(_device.batt);
  }

  void render(int x, int y) override {
    bool _invalidate = (x != this->x) || (y != this->y);
    if(_invalidate) invalidate();
    this->x = x;
    this->y = y;
    if(_invalidate) invalidate();
    render();
  }

private:
  BluetoothDevice& _device;
};

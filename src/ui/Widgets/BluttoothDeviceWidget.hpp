#pragma once
#include "display/Display.hpp"
#include "UI/Widgets/Widget.hpp"
#include "HAL/Bluetooth/Bluetooth.hpp"
#include "UI/GFX.h"

class BluetoothDeviceItem :  public Widget {
public:
  BluetoothDeviceItem(int x, int y, const BluetoothDevice& d) : Widget(x,y,128,30), device(d) {
  }

  void render() override {
    Disp::setTextSize(1);
    Disp::setCursor(x, y);
    String tempString = device.name;

    int16_t x1, y1;
    uint16_t w, h;

    Disp::getTextBounds(tempString, x, y, &x1, &y1, &w, &h);

    Disp::setTextColor(DisplayColor::WHITE);
    if (focused)
        Disp::drawRect(x - 2, y - 3, 127, 30, DisplayColor::WHITE);
    Disp::setCursor(x, y);
    Disp::print(tempString);
    Disp::drawBitmap(x, y + 10, epd_bitmap_down_right, 16, 16, DisplayColor::WHITE);

    if (device.device != nullptr) {
        Disp::drawBitmap(x + 18, y + 10, epd_bitmap_save, 16, 16, DisplayColor::WHITE);

        if (device.device->discovered())
            Disp::drawBitmap(x + 34, y + 10, epd_bitmap_Bluetooth, 16, 16, DisplayColor::WHITE);

        Disp::drawBitmap(x + 50, y + 10, epd_bitmap_battery, 32, 16, DisplayColor::WHITE);
        Disp::setCursor(x + 57, y + 15);
    }
    Disp::print(device.device->readBatt());
  }

private:
  const BluetoothDevice& device;
};

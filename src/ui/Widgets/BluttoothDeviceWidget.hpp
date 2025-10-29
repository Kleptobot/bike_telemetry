#pragma once
#include "display/Display.hpp"
#include "UI/Widgets/Widget.hpp"
#include "HAL/BluetoothInterface.hpp"
#include "UI/GFX.h"
#include "UIEventBus.hpp"
#include "DataModel.hpp"

class BluetoothDeviceWidget :  public Widget {
public:
  BluetoothDeviceWidget(int x, int y) : Widget(x,y,128,30) {}

  void update(float dt) override {
  }

  void render() override {
    if (!visible) return;

    Disp::setTextSize(1);
    Disp::setCursor(x, y);
    String tempString = _device.name;

    int16_t x1, y1;
    uint16_t w, h;

    Disp::getTextBounds(tempString, x, y, &x1, &y1, &w, &h);

    Disp::setTextColor(DispCol::WHITE);
    if (focused)
        Disp::drawRect(x - 2, y - 3, 127, 30, DispCol::WHITE);
    Disp::setCursor(x, y);
    Disp::print(tempString);
    Disp::drawBitmap(x, y + 10, epd_bitmap_down_right, 16, 16, DispCol::WHITE);

    if (_device.saved) {
      Disp::drawBitmap(x + 18, y + 10, epd_bitmap_save, 16, 16, DispCol::WHITE);

      if (_device.connected)
          Disp::drawBitmap(x + 34, y + 10, epd_bitmap_Bluetooth, 16, 16, DispCol::WHITE);

      Disp::drawBitmap(x + 50, y + 10, epd_bitmap_battery, 32, 16, DispCol::WHITE);
      Disp::setCursor(x + 57, y + 15);
    }
    Disp::print(_device.batt);
  }

    using Callback = std::function<void()>;
    virtual void setOnPress(Callback cb) { _onPress = cb; }

  void render(int x, int y) override {
      this->x = x;
      this->y = y;
      render();
  }

  void handleInput (physIO input) override {
    if (input.Select.press && focused && _onPress) {
        _onPress();  // fire callback
    }
  }

  BluetoothDevice device() const { return _device; }
  void device(BluetoothDevice* device) { _device = *device; }

private:
  BluetoothDevice _device;
  Callback _onPress;
};

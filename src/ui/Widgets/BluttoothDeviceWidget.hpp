#pragma once
#include "display/Display.hpp"
#include "UI/Widgets/Widget.hpp"
#include "HAL/BluetoothInterface.hpp"
#include "UI/GFX.h"
#include "UIEventBus.hpp"
#include "DataModel.hpp"

class BluetoothDeviceWidget :  public Widget {
public:
  using Callback = std::function<void(const BluetoothDevice&)>;
  BluetoothDeviceWidget(int x, int y, DataModel& model, const MacAddress MAC) : Widget(x,y,128,30), _model(model), _mac(MAC) {}

  void update(float dt) override {
    // Re-fetch from model by MAC to stay current
    const auto& devices = _model.bluetooth().get();
    auto it = std::find_if(devices.begin(), devices.end(), [&](const BluetoothDevice& d) { 
      return d.MAC == _mac; 
    });

    if (it != devices.end())
        _device = const_cast<BluetoothDevice*>(&(*it));
    else
        _device = nullptr;
  }

  void render() override {
    if (!visible) return;
    if (!_device) return;

    Disp::setTextSize(1);
    Disp::setCursor(x, y);
    String tempString = _device->name;

    int16_t x1, y1;
    uint16_t w, h;

    Disp::getTextBounds(tempString, x, y, &x1, &y1, &w, &h);

    Disp::setTextColor(DispCol::WHITE);
    if (focused)
        Disp::drawRect(x - 2, y - 3, 127, 30, DispCol::WHITE);
    Disp::setCursor(x, y);
    Disp::print(tempString);
    Disp::drawBitmap(x, y + 10, epd_bitmap_down_right, 16, 16, DispCol::WHITE);

    if (_device->saved) {
      Disp::drawBitmap(x + 18, y + 10, epd_bitmap_save, 16, 16, DispCol::WHITE);

      if (_device->connected)
          Disp::drawBitmap(x + 34, y + 10, epd_bitmap_Bluetooth, 16, 16, DispCol::WHITE);

      Disp::drawBitmap(x + 50, y + 10, epd_bitmap_battery, 32, 16, DispCol::WHITE);
      Disp::setCursor(x + 57, y + 15);
    }
    Disp::print(_device->batt);
  }

  void setOnPress(Callback cb) { _onPress = std::move(cb); }

  void render(int x, int y) override {
      this->x = x;
      this->y = y;
      render();
  }
  BluetoothDevice* device() const { return _device; }

private:
  DataModel& _model;
  MacAddress _mac;
  BluetoothDevice* _device = nullptr;
  Callback _onPress;
};

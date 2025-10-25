#pragma once
#include <vector>
#include "ListAdapter.hpp"
#include "UI/Widgets/BluttoothDeviceWidget.hpp"

class BluetoothWidgetAdapter : public ListAdapter {
  public:
    BluetoothWidgetAdapter(const std::vector<BluetoothDevice>& devices) : devices(devices) {
        // Build item widgets once (to avoid allocating every frame)
        for (const auto& d : devices)
          items.push_back(new BluetoothDeviceItem(d));
    }

    int getCount() const override { return items.size(); }
    UIElement* getItem(int index) override { return items[index]; }

  ~BluetoothWidgetAdapter() {
    for (auto* item : items) delete item;
  }

  private:
    const std::vector<BluetoothDevice>& devices;
    std::vector<UIElement*> items;
};
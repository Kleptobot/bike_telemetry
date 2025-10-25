#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/ListView.hpp"
#include "UI/Adapters/BluetoothWidgetAdapter.hpp"
#include "HAL/InputInterface.hpp"

class BluetoothScreen : public UIScreen {
public:
    BluetoothScreen(std::vector<BluetoothDevice>& devices) : adapter(devices), listView(&adapter, 0, 12, 128, 64) {}
    void handleInput(const physIO input) override {
        listView.handleInput(input);
    }

    void render() override {
        Disp::clear();
        Disp::drawText(0, 0, "Bluetooth Devices");
        listView.render(0,0);
        Disp::flush();
    }

private:
    BluetoothWidgetAdapter adapter;
    ListView listView;
};
#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/ListView.hpp"
#include "UI/Widgets/BluttoothDeviceWidget.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/GFX.h"
#include <memory>

class BluetoothScreen : public UIScreen {
public:
BluetoothScreen(DataModel& model)
        : UIScreen(model), listView(0, 0, 4) // 4 visible at a time
    {
        buildList();

        listView.onItemSelected([this](BluetoothDeviceWidget& w) {
            if (auto* device = w.device()) {
                emitAppEvent({AppEventType::ConnectBluetooth, *device});
            }
        });
    }

    void onEnter() override { buildList(); }

    void update(float dt) override {
        if (lastVersion != model.bluetooth().version()) {
            buildList();
        }
        lastVersion = model.bluetooth().version();
        const auto& devices = model.bluetooth().get();
        listView.update(dt);
    }

    void handleInput(physIO input) override { listView.handleInput(input); }

    void render() override { listView.render(); }

private:
    ListView<BluetoothDeviceWidget> listView;
    uint32_t lastVersion = 0;

    void buildList() {
        listView.clear();
        auto& devices = model.bluetooth().get(); // non-const for safety
        for (auto& d : devices)
            listView.addItem(std::make_unique<BluetoothDeviceWidget>(0, 0, model, d.MAC));

        lastVersion = model.bluetooth().version();
    }
};
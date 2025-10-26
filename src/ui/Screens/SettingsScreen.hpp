#pragma once
#include <vector>
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/ListView.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/GFX.h"

struct SettingsMenuItem {
    const char* label;
    const uint8_t* icon;
    ScreenID targetScreen;
};

class SettingsScreen : public UIScreen {
public:
    SettingsScreen(DataModel& model)
        : UIScreen(model),
          listView(0, 0) {
        // Define your settings options statically
        _items = {
            {"Bluetooth",   epd_bitmap_bluetooth_large,     ScreenID::Bluetooth},
            {"GPS",         epd_bitmap_antenna_large,       ScreenID::GPSSettings}, 
            {"Time",        epd_bitmap_clock,               ScreenID::TimeMenu},
            {"Biometrics",  epd_bitmap_heart_large,         ScreenID::Biometrics},
            {"Back",        epd_bitmap_left_arrow_large,    ScreenID::MainMenu}
        };

        for (const auto& i : _items)
            listView.addItem(std::make_unique<SelectableTextIconWidget>(0, 0, i.label, i.icon));

        // listView.onItemSelected([this](const SettingsMenuItem& item) {
        //     emitUIEvent(UIEventType::ChangeScreen, item.targetScreen);
        // });
    }

    void onEnter() override {
    }

    void update(float dt) override { /* nothing dynamic */ }

    void render() override {
        listView.render();
    }

    void handleInput(physIO input) override {
        listView.handleInput(input);
    }

private:
    ListView<SelectableTextIconWidget> listView;
    std::vector<SettingsMenuItem> _items;
    uint32_t _version = 0;
};

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
          listView(5, 5, 4) {
        // Define your settings options statically
        _items = {
            {"Bluetooth",   epd_bitmap_bluetooth_large,     ScreenID::Bluetooth},
            {"GPS",         epd_bitmap_antenna_large,       ScreenID::GPSSettings}, 
            {"Time",        epd_bitmap_clock_large,         ScreenID::TimeMenu},
            {"Biometrics",  epd_bitmap_heart_large,         ScreenID::Biometrics},
            {"Display",     epd_bitmap_heart_large,         ScreenID::DisplayEdit},
            {"Back",        epd_bitmap_left_arrow_large,    ScreenID::MainMenu}
        };

        for (const auto& i : _items) {
            listView.addItem(std::make_unique<SelectableTextIconWidget>(0, 0, i.label, i.icon,2,32));
        }

        listView.onItemSelected([this](auto&) {
            int index = listView.selectedIndex();
            if (index >= 0 && index < static_cast<int>(_items.size())) {
                emitUIEvent(UIEventType::ChangeScreen, _items[index].targetScreen);
            }
        });
    }

    void onEnter() override {
    }

    void update(float dt) override { listView.update(dt); }

    void render() override {
        listView.render();
    }

    void handleInput(physIO input) override {
        listView.handleInput(input);
        if (listView.selectedIndex() != _lastIndex) {
        }
        _lastIndex = listView.selectedIndex();
    }

private:
    ListView<SelectableTextIconWidget> listView;
    std::vector<SettingsMenuItem> _items;
    uint32_t _version = 0;
    int _lastIndex = 0;
};

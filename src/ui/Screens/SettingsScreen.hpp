#pragma once
#include <vector>
#include "ui/Screens/UIScreen.hpp"
#include "ui/Widgets/ListView.hpp"
#include "ui/Widgets/SelectableTextIcon.hpp"
#include "ui/Widgets/InputHints.hpp"
#include "ui/GFX.h"

#ifndef APP_BUILD_REVISION
#define APP_BUILD_REVISION "unknown"
#endif

struct SettingsMenuItem {
    const char* label;
    const uint8_t* icon;
    ScreenID targetScreen;
};

class SettingsScreen : public UIScreen {
public:
    SettingsScreen(DataModel& model)
        : UIScreen(model),
          listView(5, 5, 7) {
        // Define your settings options statically
        _items = {
            {"Bluetooth",   epd_bitmap_bluetooth_large,     ScreenID::Bluetooth},
            {"GPS",         epd_bitmap_antenna_large,       ScreenID::GPSSettings}, 
            {"Time",        epd_bitmap_clock_large,         ScreenID::TimeMenu},
            {"Biometrics",  epd_bitmap_heart_large,         ScreenID::Biometrics},
            {"Bike Stats",  epd_bitmap_sliders_32,          ScreenID::BikeStats},
            {"Logs",        epd_bitmap_running_man_32,      ScreenID::LogList},
            {"Display",     epd_bitmap_disp_edit,           ScreenID::DisplayEdit},
            {"Unmount SD",  epd_bitmap_icons8_micro_sd_32,  ScreenID::UnmountSD},
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
        listView.setIndex(0);
        hints.setHint(0, epd_bitmap_UP, "move");
        hints.setHint(1, nullptr, "choose");
    }

    void update(float dt) override { listView.update(dt); }

    void render() override {
        listView.render();
        hints.render();
        Disp::setCursor(SCREEN_WIDTH - 88, 308);
        Disp::setTextSize(1);
        Disp::setTextColor(ST77XX_WHITE);
        Disp::print("Build ");
        Disp::print(APP_BUILD_REVISION);
    }

    void handleInput(physIO input) override {
        listView.handleInput(input);
        if (listView.selectedIndex() != _lastIndex) {
        }
        _lastIndex = listView.selectedIndex();
    }

private:
    ListView<SelectableTextIconWidget> listView;
    InputHintsWidget hints{5, 300, 2};
    std::vector<SettingsMenuItem> _items;
    uint32_t _version = 0;
    int _lastIndex = 0;
};

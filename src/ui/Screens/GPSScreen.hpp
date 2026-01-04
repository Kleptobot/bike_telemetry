#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DateWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/GFX.h"

class GPSScreen : public UIScreen {
    public:
        GPSScreen (DataModel& model) : 
        UIScreen(model),
        restGPS{5,0,"Reset GPS",epd_bitmap_loop},
        restoreGPSDefaults{5,20,"Restore Defaults",epd_bitmap_gear},
        backWidget{5,40,"Back",epd_bitmap_left} {

            //attach App events
            restGPS.setOnPress([this] () {
                emitAppEvent({AppEventType::ResetGPS});
            });
            restoreGPSDefaults.setOnPress([this] () {
                emitAppEvent({AppEventType::RestoreDefaultsGPS});
            });

            //register press event callback to send a change screen event
            backWidget.setOnPress([this] () {
                emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
            });
        }

    void onEnter() override {
    }

    void render() override {
        restGPS.render();
        restoreGPSDefaults.render();
        backWidget.render();
    }

    void update(float dt) override {
        restGPS.setFocused(_index==0);
        restoreGPSDefaults.setFocused(_index==1);
        backWidget.setFocused(_index==2);
    }

    void handleInput(physIO input) override {
        if (input.Up.press) _index = (_index + 2) % 3;
        if (input.Down.press) _index = (_index + 1) % 3;

        restGPS.handleInput(input);
        restoreGPSDefaults.handleInput(input);
        backWidget.handleInput(input);
    }

    private:
        SelectableTextIconWidget restGPS;
        SelectableTextIconWidget restoreGPSDefaults;
        SelectableTextIconWidget backWidget;

        uint _index = 0;

        bool anySelected() {
            return  restGPS.isSelected() ||
                    restoreGPSDefaults.isSelected() ||
                    backWidget.isSelected();
        }

};
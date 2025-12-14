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
        backWidget{0,0,"Back",epd_bitmap_left} {

            //register press event callback to send a change screen event
            backWidget.setOnPress([this] () {
                emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
            });
        }

    void onEnter() override {
        backWidget.setFocused(true);
    }

    void render() override {
        backWidget.render();
    }

    void update(float dt) override {}

    void handleInput(physIO input) override {
        backWidget.handleInput(input);
    }

    private:
        SelectableTextIconWidget backWidget;

};
#pragma once

#include <string>
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DateWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/GFX.h"
#include "Display/Display.hpp"

class BikeStatsScreen : public UIScreen {
    public:
        BikeStatsScreen (DataModel& model) : 
            UIScreen(model),
            bikeMassLabel(5,5,"Bike Mass:"),
            bikeMassWidget{bikeMassLabel.width() + 10,5, String(_mass/10.0f,1) + " kg"},
            wheelCircLabel{5,32,"Wheel C:"},
            wheelCircWidget{wheelCircLabel.width() + 10,32, String(_circumference) + " mm"},

            backWidget{15,231,"Back",epd_bitmap_left},
            saveWidget{90,231,"Save",epd_bitmap_save} {
                //register press event callback to send a change screen event
                backWidget.setOnPress([this] () {
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
                //register the save press event callback to send a change screen and app save event
                saveWidget.setOnPress([this] () {
                    this->model.bike().update({(_mass),(_circumference)});
                    emitAppEvent({AppEventType::SaveBikeStats,0});
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
            }
        void onEnter() override {
            auto& a = model.bike().get();

            _mass = (a.mass == 0 ? 130 : a.mass);
            _circumference = (a.wheelCircumference == 0 ? 2136 : a.wheelCircumference);
        }

        void update(float dt) override {
            bikeMassWidget.setText(String(_mass/10.0f,1) + " kg");
            wheelCircWidget.setText(String(_circumference) + " mm");

            bikeMassWidget.setFocused(focusField == EditField::Mass);
            wheelCircWidget.setFocused(focusField == EditField::WheelCircumference);

            backWidget.setFocused(focusField == EditField::Back);
            saveWidget.setFocused(focusField == EditField::Save);
        }

        void handleInput(physIO input) override;

        void render() override {
            bikeMassLabel.render();
            bikeMassWidget.render();

            wheelCircLabel.render();
            wheelCircWidget.render();

            backWidget.render();
            saveWidget.render();
        }

    private:
        enum class EditField { Mass, WheelCircumference, Back, Save };
        EditField focusField = EditField::Mass;

        SelectableTextWidget bikeMassLabel;
        SelectableTextWidget bikeMassWidget;

        SelectableTextWidget wheelCircLabel;
        SelectableTextWidget wheelCircWidget;

        SelectableTextIconWidget backWidget;
        SelectableTextIconWidget saveWidget;
        
        uint16_t _mass;
        uint16_t _circumference;
        uint16_t _repeatCount = 0;

        void moveFocusUp();
        void moveFocusDown();
        void moveFocusLeft();
        void moveFocusRight();

        bool anySelected() {return bikeMassWidget.isSelected() ||
                                    wheelCircWidget.isSelected(); }

};
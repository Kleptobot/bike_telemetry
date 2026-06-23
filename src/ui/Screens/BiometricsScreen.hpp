#pragma once
#include <string>
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DateWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/GFX.h"
#include "Display/Display.hpp"

class BiometricsScreen : public UIScreen {
    public:
        BiometricsScreen (DataModel& model) : 
            UIScreen(model),
            birthdayLabel(5,5,"Birthday:"),
            dateWidget{110,5, &_birthday},
            massLabel(5,32,"Mass:"),
            massWidget(110,32, String(0)),
            calorieLabel(5,59,"Calory Calc:"),
            calorieWidget(calorieLabel.width() + 10,59, "-"),

            zone1StartLabel(5,91,"Zone 1 Start:"),
            zone1StartWidget(zone1StartLabel.width() + 10,91, "99"),    //55% of 180 is 99
            zone2StartLabel(5,118,"Zone 2 Start:"),
            zone2StartWidget(zone2StartLabel.width() + 10,118, "117"),  //65% of 180 is 117
            zone3StartLabel(5,145,"Zone 3 Start:"),
            zone3StartWidget(zone3StartLabel.width() + 10,145, "138"),  //77% of 180 is 138
            zone4StartLabel(5,172,"Zone 4 Start:"),
            zone4StartWidget(zone4StartLabel.width() + 10,172, "151"),  //84% of 180 is 151
            zone5StartLabel(5,199,"Zone 5 Start:"),
            zone5StartWidget(zone5StartLabel.width() + 10,199, "162"),  //90% of 180 is 162


            backWidget{15,231,"Back",epd_bitmap_left},
            saveWidget{90,231,"Save",epd_bitmap_save} {
                //register press event callback to send a change screen event
                backWidget.setOnPress([this] () {
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
                //register the save press event callback to send a change screen and app save event
                saveWidget.setOnPress([this] () {
                    this->model.app().update({AppState::IDLE,_birthday,_mass,_caloricProfile,_zone1Start,_zone2Start,_zone3Start,_zone4Start,_zone5Start});
                    emitAppEvent({AppEventType::SaveBiometrics,0});
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
            }
        void onEnter() override {
            auto& a = model.app().get();
            _birthday = a.birthday;
            _mass = a.mass;
            _caloricProfile = a.caloricProfile;
            _zone1Start = a.zone1Start;
            _zone2Start = a.zone2Start;
            _zone3Start = a.zone3Start;
            _zone4Start = a.zone4Start;
            _zone5Start = a.zone5Start;
        }

        void update(float dt) override {
            dateWidget.update(dt);
            massWidget.setText(String(_mass));
            switch(_caloricProfile) {
                case CaloricProfile::Female:
                    calorieWidget.setText("F");
                    break;
                case CaloricProfile::Male:
                    calorieWidget.setText("M");
                    break;
                case CaloricProfile::Other:
                    calorieWidget.setText("-");
                    break;
            }
            zone1StartWidget.setText(String(_zone1Start));
            zone2StartWidget.setText(String(_zone2Start));
            zone3StartWidget.setText(String(_zone3Start));
            zone4StartWidget.setText(String(_zone4Start));
            zone5StartWidget.setText(String(_zone5Start));

            dateWidget.setFocused(focusField == EditField::Birthday);
            massWidget.setFocused(focusField == EditField::Mass);
            calorieWidget.setFocused(focusField == EditField::CaloricProfile);
            zone1StartWidget.setFocused(focusField == EditField::Zone1Start);
            zone2StartWidget.setFocused(focusField == EditField::Zone2Start);
            zone3StartWidget.setFocused(focusField == EditField::Zone3Start);
            zone4StartWidget.setFocused(focusField == EditField::Zone4Start);
            zone5StartWidget.setFocused(focusField == EditField::Zone5Start);
            backWidget.setFocused(focusField == EditField::Back);
            saveWidget.setFocused(focusField == EditField::Save);
        }

        void handleInput(physIO input) override;

        void render() override {
            birthdayLabel.render();
            dateWidget.render();

            massLabel.render();
            massWidget.render();

            calorieLabel.render();
            calorieWidget.render();

            zone1StartLabel.render();
            zone1StartWidget.render();

            zone2StartLabel.render();
            zone2StartWidget.render();

            zone3StartLabel.render();
            zone3StartWidget.render();

            zone4StartLabel.render();
            zone4StartWidget.render();

            zone5StartLabel.render();
            zone5StartWidget.render();

            backWidget.render();
            saveWidget.render();
        }

    private:
        enum class EditField { Birthday = 0, Mass, CaloricProfile, Zone1Start, Zone2Start, Zone3Start, Zone4Start, Zone5Start, Back, Save };
        EditField focusField = EditField::Birthday;

        SelectableTextWidget birthdayLabel;
        DateWidget dateWidget;

        SelectableTextWidget massLabel;
        SelectableTextWidget massWidget;

        SelectableTextWidget calorieLabel;
        SelectableTextWidget calorieWidget;
        SelectableTextWidget zone1StartLabel;
        SelectableTextWidget zone1StartWidget;
        SelectableTextWidget zone2StartLabel;
        SelectableTextWidget zone2StartWidget;
        SelectableTextWidget zone3StartLabel;
        SelectableTextWidget zone3StartWidget;
        SelectableTextWidget zone4StartLabel;
        SelectableTextWidget zone4StartWidget;
        SelectableTextWidget zone5StartLabel;
        SelectableTextWidget zone5StartWidget;

        SelectableTextIconWidget backWidget;
        SelectableTextIconWidget saveWidget;
        timeData _birthday;
        uint16_t _mass;
        CaloricProfile _caloricProfile;
        uint8_t _zone1Start;
        uint8_t _zone2Start;
        uint8_t _zone3Start;
        uint8_t _zone4Start;
        uint8_t _zone5Start;

        void moveFocusUp();
        void moveFocusDown();
        void moveFocusLeft();
        void moveFocusRight();

        bool anySelected() {return dateWidget.isSelected() ||
                                    massWidget.isSelected() ||
                                    calorieWidget.isSelected() ||
                                    saveWidget.isSelected() ||
                                    zone1StartWidget.isSelected() ||
                                    zone2StartWidget.isSelected() ||
                                    zone3StartWidget.isSelected() ||
                                    zone4StartWidget.isSelected() ||
                                    zone5StartWidget.isSelected() ||
                                    backWidget.isSelected(); }

};
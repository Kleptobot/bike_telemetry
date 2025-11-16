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
            birthdayLabel(5,5,"Birthday"),
            dateWidget{110,5, &_birthday},
            massLabel(5,32,"mass"),
            massWidget(110,32, String(0)),
            calorieLabel(5,64,"Gender"),
            calorieWidget(110,64, "-"),
            backWidget{15,96,"Back",epd_bitmap_left},
            saveWidget{90,96,"Save",epd_bitmap_save} {
                //register press event callback to send a change screen event
                backWidget.setOnPress([this] () {
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
                //register the save press event callback to send a change screen and app save event
                saveWidget.setOnPress([this] () {
                    this->model.app().update({AppState::IDLE,_birthday,_mass,_caloricProfile});
                    emitAppEvent({AppEventType::SaveBiometrics,0});
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
            }
        void onEnter() override {
            auto& a = model.app().get();
            _birthday = a.birthday;
            _mass = a.mass;
            _caloricProfile = a.caloricProfile;
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
            dateWidget.setFocused(focusField == EditField::Birthday);
            massWidget.setFocused(focusField == EditField::Mass);
            calorieWidget.setFocused(focusField == EditField::CaloricProfile);
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

            backWidget.render();
            saveWidget.render();
        }

    private:
        enum class EditField { Birthday = 0, Mass, CaloricProfile, Back, Save };
        EditField focusField = EditField::Birthday;

        SelectableTextWidget birthdayLabel;
        DateWidget dateWidget;

        SelectableTextWidget massLabel;
        SelectableTextWidget massWidget;

        SelectableTextWidget calorieLabel;
        SelectableTextWidget calorieWidget;

        SelectableTextIconWidget backWidget;
        SelectableTextIconWidget saveWidget;
        DateTime _birthday;
        uint16_t _mass;
        CaloricProfile _caloricProfile;

        void moveFocusUp();
        void moveFocusDown();
        void moveFocusLeft();
        void moveFocusRight();

        bool anySelected() {return dateWidget.isSelected() ||
                                    massWidget.isSelected() ||
                                    calorieWidget.isSelected() ||
                                    saveWidget.isSelected() ||
                                    backWidget.isSelected(); }

};
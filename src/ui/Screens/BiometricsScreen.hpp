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
            dateWidget{16,0, &_birthday},
            massWidget(16,32, ""),
            calorieWidget(16,64, ""),
            backWidget{0,96,"Back",epd_bitmap_left},
            saveWidget{64,96,"Save",epd_bitmap_save} {
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
        }

        void handleInput(physIO input) override;

        void render() override {
            Disp::print("Birthday: ");
            dateWidget.render();

            Disp::print("Mass: ");
            massWidget.render();
            calorieWidget.render();

            backWidget.render();
            saveWidget.render();
        }

    private:
        enum class EditField { Birthday = 0, Mass, CaloricProfile, Back, Save };
        EditField focusField = EditField::Birthday;
        DateWidget dateWidget;
        SelectableTextWidget massWidget;
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
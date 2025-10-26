#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DateWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/GFX.h"
#include "App.hpp"

class TimeEditScreen : public UIScreen {
    public:
        TimeEditScreen (DataModel& model) : 
            UIScreen(model),
            timeWidget{0,0},
            dateWidget{0,0},
            backWidget{0,48,"Back",epd_bitmap_left},
            saveWidget{64,48,"Save",epd_bitmap_save} {
                //register press event callback to send a change screen event
                backWidget.setOnPress([this] () {
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
                //register the save press event callback to send a change screen and app save event
                saveWidget.setOnPress([this] () {
                    this->model.time().update(this->_date);
                    emitAppEvent({AppEventType::SaveTime,0});
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
            }
        void onEnter() override {
            _date = model.time().get();
            dateWidget.setDate(_date);
            timeWidget.setDate(_date);
        }

        void update(float dt) override;

        void handleInput(physIO input) override;

        void render() override {
            timeWidget.render();
            dateWidget.render();
            backWidget.render();
            saveWidget.render();
        }

    protected:
        DateTime _date;
    private:
        enum class EditField { Time = 0, Date, Back, Save };
        EditField focusField = EditField::Time;
        TimeWidget timeWidget;
        DateWidget dateWidget;
        SelectableTextIconWidget backWidget;
        SelectableTextIconWidget saveWidget;

        void moveFocusUp();
        void moveFocusDown();
        void moveFocusLeft();
        void moveFocusRight();
        bool anySelected() {return timeWidget.isSelected() ||
                                    dateWidget.isSelected() ||
                                    saveWidget.isSelected() ||
                                    backWidget.isSelected(); }

};
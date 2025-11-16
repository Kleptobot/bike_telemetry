#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DateWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/GFX.h"

class TimeEditScreen : public UIScreen {
    public:
        TimeEditScreen (DataModel& model) : 
            UIScreen(model),
            timeWidget{5,5, &_date},
            dateWidget{5,30, &_date},
            backWidget{5,55,"Back",epd_bitmap_left},
            saveWidget{75,55,"Save",epd_bitmap_save} {
                //register press event callback to send a change screen event
                backWidget.setOnPress([this] () {
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
                //register the save press event callback to send a change screen and app save event
                saveWidget.setOnPress([this] () {
                    this->model.time().update(this->_date);
                    emitAppEvent({AppEventType::SaveTime,_date});
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
            }
        void onEnter() override {
            _date = model.time().get();
        }

        void update(float dt) override;

        void handleInput(physIO input) override;

        void render() override {
            timeWidget.render();
            dateWidget.render();
            backWidget.render();
            saveWidget.render();
        }

    private:
        enum class EditField { Time = 0, Date, Back, Save };
        EditField focusField = EditField::Time;
        TimeWidget timeWidget;
        DateWidget dateWidget;
        SelectableTextIconWidget backWidget;
        SelectableTextIconWidget saveWidget;
        DateTime _date;

        void moveFocusUp();
        void moveFocusDown();
        void moveFocusLeft();
        void moveFocusRight();
        bool anySelected() {return timeWidget.isSelected() ||
                                    dateWidget.isSelected() ||
                                    saveWidget.isSelected() ||
                                    backWidget.isSelected(); }

};
#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DateWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/Widgets/InputHints.hpp"
#include "UI/GFX.h"

class TimeEditScreen : public UIScreen {
    public:
        TimeEditScreen (DataModel& model) : 
            UIScreen(model),
            timeWidget{5,5, _date},
            dateWidget{5,30, _date},
            UTCOffsetLabel{5,55, "UTC offset: "},
            UTCOffsetDisp{UTCOffsetLabel.width() + 10,55, ""},
            backWidget{5,87,"Back",epd_bitmap_left},
            saveWidget{75,87,"Save",epd_bitmap_save} {
                //register press event callback to send a change screen event
                backWidget.setOnPress([this] () {
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
                //register the save press event callback to send a change screen and app save event
                saveWidget.setOnPress([this] () {
                    this->model.time().update(this->_date);
                    emitAppEvent({AppEventType::SaveTime,_date});
                    emitAppEvent({AppEventType::SaveBiometrics,0});
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
            }
        void onEnter() override {
            _date = model.time().get();
            hints.setHint(0, epd_bitmap_left, "back");
            hints.setHint(1, nullptr, "edit");
            hints.setHint(2, epd_bitmap_save, "save");
        }

        void update(float dt) override;

        void handleInput(physIO input) override;

        void render() override {
            timeWidget.render();
            dateWidget.render();
            UTCOffsetLabel.render();
            UTCOffsetDisp.render();
            backWidget.render();
            saveWidget.render();
            hints.render();
        }

    private:
        enum class EditField { Time = 0, Date, UTC,  Back, Save };
        EditField focusField = EditField::Time;
        TimeWidget timeWidget;
        DateWidget dateWidget;
        SelectableTextWidget UTCOffsetLabel;
        SelectableTextWidget UTCOffsetDisp;
        SelectableTextIconWidget backWidget;
        SelectableTextIconWidget saveWidget;
        timeData _date;
        InputHintsWidget hints{5, 298, 3};

        void moveFocusUp();
        void moveFocusDown();
        void moveFocusLeft();
        void moveFocusRight();
        bool anySelected() {return timeWidget.isSelected() ||
                                    dateWidget.isSelected() ||
                                    UTCOffsetDisp.isSelected() ||
                                    saveWidget.isSelected() ||
                                    backWidget.isSelected(); }

};
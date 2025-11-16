#pragma once
#include <RTClib.h>

#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"

class DateWidget : public Widget {
    public:
        DateWidget(int x, int y, DateTime* date=nullptr)
            : Widget(x, y),
            _date(date),
            dayText(x, y, ""),
            monthText(x + 45, y, ""),
            yearText(x + 90, y, "") {}

        void render() override;
        void handleInput(physIO input);

        void update(float dt) override;

        void setDate(DateTime* date) { 
            _date=date;
        }

        bool isSelected() const override { return dayText.isSelected() ||
                                            monthText.isSelected() ||
                                            yearText.isSelected();}

        void setFocused(bool f) override {
            if( focusField == EditField::None) {
                if(f) focusField = EditField::Day;
            }
            else {
                if(!f) focusField = EditField::None;
            }
            focused = f;
        }
        DateTime getDate() const { return *_date; }

    private:
        enum class EditField { None = 0, Day, Month, Year };
        EditField focusField = EditField::Day;
        DateTime* _date;

        SelectableTextWidget dayText;
        SelectableTextWidget monthText;
        SelectableTextWidget yearText;

        void incrementField(EditField field);
        void decrementField(EditField field);
        void moveFocusLeft();
        void moveFocusRight();
};

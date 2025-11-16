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
            monthText(x + 12*3, y, ""),
            yearText(x + 12*6, y, "") {
            _width = 12*10;
            _height = 16;
        }

        void render() override;
        void handleInput(physIO input);

        void update(float dt) override;

        void setDate(DateTime* date) { 
            _date=date;
        }

        bool isSelected() const override { return dayText.isSelected() ||
                                            monthText.isSelected() ||
                                            yearText.isSelected();}

        DateTime getDate() const { return *_date; }

    private:
        enum class EditField { Day, Month, Year };
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

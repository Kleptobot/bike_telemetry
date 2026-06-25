#pragma once
#include <RTClib.h>

#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"
#include "DataModel/TimeDataProvider.hpp"

class DateWidget : public Widget {
    public:
        DateWidget(int x, int y, timeData& date)
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

        bool isSelected() const override { return dayText.isSelected() ||
                                            monthText.isSelected() ||
                                            yearText.isSelected();}

    private:
        enum class EditField { Day, Month, Year };
        EditField focusField = EditField::Day;
        timeData& _date;

        SelectableTextWidget dayText;
        SelectableTextWidget monthText;
        SelectableTextWidget yearText;

        void editField(EditField field, int val);
        void moveFocusLeft();
        void moveFocusRight();
};

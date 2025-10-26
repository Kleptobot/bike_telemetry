#pragma once
#include <RTClib.h>

#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"

class DateWidget : public Widget {
    public:
        DateWidget(int x, int y)
            : Widget(x, y),
            dayText(x, y, ""),
            monthText(x + 45, y, ""),
            yearText(x + 90, y, "") {}

        void render() override;
        void handleInput(physIO input);

        void setDate(DateTime date) { _date=date; }
        DateTime getDate() const { return _date; }

    private:
        enum class EditField { None = 0, Day, Month, Year };
        DateTime _date;
        EditField focusField = EditField::Day;

        SelectableTextWidget dayText;
        SelectableTextWidget monthText;
        SelectableTextWidget yearText;

        void incrementField(EditField field);
        void decrementField(EditField field);
        void moveFocusLeft();
        void moveFocusRight();
};

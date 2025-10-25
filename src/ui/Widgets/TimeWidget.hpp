#pragma once
#include <RTClib.h>

#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"

class TimeWidget : public Widget {
public:
    TimeWidget(int x, int y)
        : Widget(x, y),
          hourText(x, y, ""),
          minuteText(x + 45, y, ""),
          secondText(x + 90, y, "") {}

    void render() override;
    void handleInput(physIO input);

    void setDate(DateTime date) { _date=date; }
    DateTime getDate() const { return _date; }

private:
    enum class EditField { None = 0, Hour, Minute, Second };
    DateTime _date;
    EditField focusField = EditField::Hour;

    SelectableTextWidget hourText;
    SelectableTextWidget minuteText;
    SelectableTextWidget secondText;

    void incrementField(EditField field);
    void decrementField(EditField field);
    void moveFocusLeft();
    void moveFocusRight();
};

#pragma once
#include <RTClib.h>

#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"

class TimeWidget : public Widget {
public:
    TimeWidget(int x, int y, DateTime* date=nullptr)
        : Widget(x, y),
          _date(date),
          hourText(x, y, ""),
          minuteText(x + 45, y, ""),
          secondText(x + 90, y, "") {}

    void render() override;
    void handleInput(physIO input);

    bool isSelected() const override { return hourText.isSelected() ||
                                        minuteText.isSelected() ||
                                        secondText.isSelected();}

    void setFocused(bool f) override {
        if( focusField == EditField::None) {
            if(f) focusField = EditField::Hour;
        }
        else {
            if(!f) focusField = EditField::None;
        }
        focused = f;
    }

    void setDate(DateTime* date) { 
        _date=date;
        hourText.setText(String( _date->hour()));
        minuteText.setText(String( _date->minute()));
        secondText.setText(String( _date->second()));
    }
    DateTime getDate() const { return *_date; }

    void update(float dt) override;

private:
    enum class EditField { None = 0, Hour, Minute, Second };
    EditField focusField = EditField::None;
    DateTime *_date;

    SelectableTextWidget hourText;
    SelectableTextWidget minuteText;
    SelectableTextWidget secondText;

    void incrementField(EditField field);
    void decrementField(EditField field);
    void moveFocusLeft();
    void moveFocusRight();
};

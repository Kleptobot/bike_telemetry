#pragma once
#include <RTClib.h>

#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"
#include "DataModel/TimeDataProvider.hpp"

class TimeWidget : public Widget {
public:
    TimeWidget(int x, int y, timeData* date=nullptr)
        : Widget(x, y),
          _date(date),
          hourText(x, y, ""),
          minuteText(x + 12*3, y, ""),
          secondText(x + 12*6, y, "") {
            _width = 12*10;
            _height = 16;
          }

    void render() override;
    void handleInput(physIO input);

    bool isSelected() const override { return hourText.isSelected() ||
                                        minuteText.isSelected() ||
                                        secondText.isSelected();}

    void update(float dt) override;

private:
    enum class EditField { Hour, Minute, Second };
    EditField focusField = EditField::Hour;
    timeData* _date;

    SelectableTextWidget hourText;
    SelectableTextWidget minuteText;;
    SelectableTextWidget secondText;

    void editField(EditField field, int val);
    void moveFocusLeft();
    void moveFocusRight();
};

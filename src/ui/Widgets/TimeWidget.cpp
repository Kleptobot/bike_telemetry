#include "TimeWidget.hpp"
#include "Display/Display.hpp"

void TimeWidget::handleInput(physIO input) {

    if (input.Select.press) {
        selected = !selected;
    }

    if (selected) {
        if (input.Up.press)   incrementField(focusField);
        if (input.Down.press) decrementField(focusField);
    } else {
        if (input.Left.press)  moveFocusLeft();
        if (input.Right.press) moveFocusRight();
    }
}

void TimeWidget::moveFocusLeft() {
    if (selected) return; // can't move focus while editing
    switch (focusField) {
        case EditField::Hour:   focusField = EditField::Second; break;
        case EditField::Minute: focusField = EditField::Hour;  break;
        case EditField::Second:  focusField = EditField::Minute; break;
        default: break;
    }
}

void TimeWidget::moveFocusRight() {
    if (selected) return;
    switch (focusField) {
        case EditField::Hour:   focusField = EditField::Minute; break;
        case EditField::Minute: focusField = EditField::Second;  break;
        case EditField::Second:  focusField = EditField::Hour;   break;
        default: break;
    }
}

void TimeWidget::incrementField(EditField field) {
    switch (field) {
        case EditField::Hour:
            _date = DateTime(_date.second(), _date.minute(),
                            (_date.hour() + 1) % 24,
                            _date.hour(), _date.minute(), _date.second());
            break;

        case EditField::Minute:
            _date = DateTime(_date.second(),
                            (_date.minute() +1) % 60,
                            _date.hour(),
                            _date.hour(), _date.minute(), _date.second());
            break;

        case EditField::Second:
            _date = DateTime((_date.second() + 1) % 60,
                            _date.minute(), 
                            _date.hour(),
                            _date.hour(), _date.minute(), _date.second());
            break;

        default: break;
    }
}

void TimeWidget::decrementField(EditField field) {
    switch (field) {
        case EditField::Hour:
            _date = DateTime(_date.second(), _date.minute(),
                            (_date.hour() == 1 ? 23 : _date.hour() - 1),
                            _date.hour(), _date.minute(), _date.second());
            break;

        case EditField::Minute:
            _date = DateTime(_date.second(),
                            (_date.minute() == 1 ? 59 : _date.minute() - 1),
                            _date.hour(),
                            _date.hour(), _date.minute(), _date.second());
            break;

        case EditField::Second:
            _date = DateTime((_date.second() == 1 ? 60 : _date.second() - 1), 
                            _date.minute(), 
                            _date.hour(),
                            _date.hour(), _date.minute(), _date.second());
            break;

        default: break;
    }
}

void TimeWidget::render() {
    // update field visuals
    hourText.setFocused(focusField == EditField::Hour && focused);
    hourText.setSelected(selected && focusField == EditField::Hour);

    minuteText.setFocused(focusField == EditField::Minute && focused);
    minuteText.setSelected(selected && focusField == EditField::Minute);

    secondText.setFocused(focusField == EditField::Second && focused);
    secondText.setSelected(selected && focusField == EditField::Second);

    // draw them
    hourText.render();
    minuteText.render();
    secondText.render();
}

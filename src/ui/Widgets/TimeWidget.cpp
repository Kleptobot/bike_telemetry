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

void TimeWidget::update(float dt) {
    hourText.setFocused(focusField == EditField::Hour && focused);
    minuteText.setFocused(focusField == EditField::Minute && focused);
    secondText.setFocused(focusField == EditField::Second && focused);

    hourText.setSelected(focusField == EditField::Hour && selected);
    minuteText.setSelected(focusField == EditField::Minute && selected);
    secondText.setSelected(focusField == EditField::Second && selected);
            
    hourText.setText(String( _date->hour()));
    minuteText.setText(String( _date->minute()));
    secondText.setText(String( _date->second()));
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
            *_date = DateTime(_date->year(), _date->month(), _date->day(), 
                            (_date->hour() + 1) % 24,
                            _date->minute(),
                            _date->second());
            hourText.setText(String( _date->hour()));
            break;

        case EditField::Minute:
            *_date = DateTime(_date->year(), _date->month(), _date->day(), 
                            _date->hour(),
                            (_date->minute() +1) % 60,
                            _date->second());
            minuteText.setText(String( _date->minute()));
            break;

        case EditField::Second:
            *_date = DateTime(_date->year(), _date->month(), _date->day(), 
                            _date->minute(), 
                            _date->hour(),
                            (_date->second() + 1) % 60);
            secondText.setText(String( _date->second()));
            break;

        default: break;
    }
}

void TimeWidget::decrementField(EditField field) {
    switch (field) {
        case EditField::Hour:
            *_date = DateTime(_date->year(), _date->month(), _date->day(),
                            (_date->hour() == 1 ? 23 : _date->hour() - 1),
                            _date->minute(),
                            _date->second());
            hourText.setText(String( _date->hour()));
            break;

        case EditField::Minute:
            *_date = DateTime(_date->year(), _date->month(), _date->day(),
                            _date->hour(),
                            (_date->minute() == 1 ? 59 : _date->minute() - 1),
                            _date->second());
            minuteText.setText(String( _date->minute()));
            break;

        case EditField::Second:
            *_date = DateTime(_date->year(), _date->month(), _date->day(),
                            _date->hour(),
                            _date->minute(), 
                            (_date->second() == 1 ? 60 : _date->second() - 1));
            secondText.setText(String( _date->second()));
            break;

        default: break;
    }
}

void TimeWidget::render() {
    if (!visible) return;

    // draw them
    hourText.render();
    minuteText.render();
    secondText.render();
}

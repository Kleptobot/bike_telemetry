#include "DateWidget.hpp"
#include "Display/Display.hpp"

void DateWidget::handleInput(physIO input) {

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

void DateWidget::moveFocusLeft() {
    if (selected) return; // can't move focus while editing
    switch (focusField) {
        case EditField::Day:   focusField = EditField::Year; break;
        case EditField::Month: focusField = EditField::Day;  break;
        case EditField::Year:  focusField = EditField::Month; break;
        default: break;
    }
}

void DateWidget::moveFocusRight() {
    if (selected) return;
    switch (focusField) {
        case EditField::Day:   focusField = EditField::Month; break;
        case EditField::Month: focusField = EditField::Year;  break;
        case EditField::Year:  focusField = EditField::Day;   break;
        default: break;
    }
}

void DateWidget::incrementField(EditField field) {
    switch (field) {
        case EditField::Day:
            _date = DateTime(_date.year(), _date.month(),
                            (_date.day() % 31) + 1,
                            _date.hour(), _date.minute(), _date.second());
            dayText.setText(String( _date.day()));
            break;

        case EditField::Month:
            _date = DateTime(_date.year(),
                            (_date.month() % 12) + 1,
                            _date.day(),
                            _date.hour(), _date.minute(), _date.second());
            monthText.setText(String( _date.month()));
            break;

        case EditField::Year:
            _date = DateTime(_date.year() + 1, _date.month(), _date.day(),
                            _date.hour(), _date.minute(), _date.second());
            yearText.setText(String( _date.year()));
            break;

        default: break;
    }
}

void DateWidget::decrementField(EditField field) {
    switch (field) {
        case EditField::Day:
            _date = DateTime(_date.year(), _date.month(),
                            (_date.day() == 1 ? 31 : _date.day() - 1),
                            _date.hour(), _date.minute(), _date.second());
            dayText.setText(String( _date.day()));
            break;

        case EditField::Month:
            _date = DateTime(_date.year(),
                            (_date.month() == 1 ? 12 : _date.month() - 1),
                            _date.day(),
                            _date.hour(), _date.minute(), _date.second());
            monthText.setText(String( _date.month()));
            break;

        case EditField::Year:
            _date = DateTime(_date.year() - 1, _date.month(), _date.day(),
                            _date.hour(), _date.minute(), _date.second());
            yearText.setText(String( _date.year()));
            break;

        default: break;
    }
}

void DateWidget::render() {
    if (!visible) return;
    // update field visuals
    dayText.setFocused(focusField == EditField::Day && focused);
    dayText.setSelected(selected && focusField == EditField::Day);

    monthText.setFocused(focusField == EditField::Month && focused);
    monthText.setSelected(selected && focusField == EditField::Month);

    yearText.setFocused(focusField == EditField::Year && focused);
    yearText.setSelected(selected && focusField == EditField::Year);

    // draw them
    dayText.render();
    monthText.render();
    yearText.render();
}

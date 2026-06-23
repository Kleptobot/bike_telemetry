#include "TimeEditScreen.hpp"

void TimeEditScreen::update(float dt) {
    timeWidget.update(dt);
    dateWidget.update(dt);
    UTCOffsetDisp.setText(String((float)_date.offset()/60.0));

    timeWidget.setFocused (focusField == EditField::Time);
    dateWidget.setFocused (focusField == EditField::Date);
    UTCOffsetDisp.setFocused (focusField == EditField::UTC);
    backWidget.setFocused (focusField == EditField::Back);
    saveWidget.setFocused (focusField == EditField::Save);


}

void TimeEditScreen::handleInput(physIO input) {
    if (!anySelected()) {
        if (input.Up.press)
            moveFocusUp();
        if (input.Down.press)
            moveFocusDown();
        if (input.Left.press)
            moveFocusLeft();
        if (input.Right.press)
            moveFocusRight();
    }
    switch (focusField) {
        case EditField::Time:
            timeWidget.handleInput(input);
            break;
        case EditField::Date:
            dateWidget.handleInput(input);
            break;
        case EditField::UTC:
        // Handle Up: press or held with repeat
            if (UTCOffsetDisp.isSelected()) {
                if (input.Up.state && UTCOffsetDisp.shouldRepeat(input.Up.heldTime)) {
                    _date.add_offset(30);
                }else if (input.Up.press) {
                    _date.add_offset(30);
                } 
                // Handle Down: press or held with repeat
                if (input.Down.state && UTCOffsetDisp.shouldRepeat(input.Down.heldTime)) {
                    _date.add_offset(-30);
                } else if (input.Down.press) {
                    _date.add_offset(-30);
                }
            }
            if (input.Select.press) UTCOffsetDisp.setSelected(!UTCOffsetDisp.isSelected());
            break;
        case EditField::Back:
            backWidget.handleInput(input);
            break;
        case EditField::Save:
            saveWidget.handleInput(input);
            break;
        default: break;
    }
}

void TimeEditScreen::moveFocusUp() {
    switch (focusField) {
        case EditField::Time: focusField = EditField::Back; break;
        case EditField::Date: focusField = EditField::Time; break;
        case EditField::UTC: focusField = EditField::Date; break;
        case EditField::Back: focusField = EditField::UTC; break;
        case EditField::Save: focusField = EditField::UTC; break;
        default: break;
    }
}

void TimeEditScreen::moveFocusDown() {
    switch (focusField) {
        case EditField::Time: focusField = EditField::Date; break;
        case EditField::Date: focusField = EditField::UTC; break;
        case EditField::UTC: focusField = EditField::Back; break;
        case EditField::Back: focusField = EditField::Time; break;
        case EditField::Save: focusField = EditField::Time; break;
        default: break;
    }
}

void TimeEditScreen::moveFocusLeft() {
    switch (focusField) {
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Back; break;
        default: break;
    }
}

void TimeEditScreen::moveFocusRight() {
    switch (focusField) {
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Back; break;
        default: break;
    }
}
#include "TimeEditScreen.hpp"

void TimeEditScreen::update(float dt) {
    switch (focusField) {
        case EditField::Time:
            _date = timeWidget.getDate();
            break;

        case EditField::Date:
            _date = dateWidget.getDate();
            break;
        
        default:
            break;
    }

    timeWidget.setFocused (focusField == EditField::Time);
    dateWidget.setFocused (focusField == EditField::Date);
    
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
        case EditField::Back: focusField = EditField::Date; break;
        case EditField::Save: focusField = EditField::Date; break;
        default: break;
    }
}

void TimeEditScreen::moveFocusDown() {
    switch (focusField) {
        case EditField::Time: focusField = EditField::Date; break;
        case EditField::Date: focusField = EditField::Back; break;
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
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
    } else {
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
}

void TimeEditScreen::moveFocusUp() {
    switch (focusField) {
        case EditField::Time:
            focusField = EditField::Back;
            timeWidget.setFocused(false);
            backWidget.setFocused(true);
            break;
        case EditField::Date:
            focusField = EditField::Time;
            dateWidget.setFocused(false);
            timeWidget.setFocused(true);
            break;
        case EditField::Back:
            focusField = EditField::Date;
            backWidget.setFocused(false);
            dateWidget.setFocused(true);
            break;
        case EditField::Save:
            focusField = EditField::Date;
            saveWidget.setFocused(false);
            dateWidget.setFocused(true);
            break;
        default: break;
    }
}

void TimeEditScreen::moveFocusDown() {
    switch (focusField) {
        case EditField::Time:
            focusField = EditField::Date;
            timeWidget.setFocused(false);
            dateWidget.setFocused(true);
            break;
        case EditField::Date:
            focusField = EditField::Back;
            dateWidget.setFocused(false);
            backWidget.setFocused(true);
            break;
        case EditField::Back:
            focusField = EditField::Time;
            backWidget.setFocused(false);
            timeWidget.setFocused(true);
            break;
        case EditField::Save:
            focusField = EditField::Time;
            saveWidget.setFocused(false);
            timeWidget.setFocused(true);
            break;
        default: break;
    }
}

void TimeEditScreen::moveFocusLeft() {
    switch (focusField) {
        case EditField::Back:
            focusField = EditField::Save;
            backWidget.setFocused(false);
            saveWidget.setFocused(true);
            break;
        case EditField::Save:
            focusField = EditField::Back;
            backWidget.setFocused(true);
            saveWidget.setFocused(false);
            break;
        default: break;
    }
}

void TimeEditScreen::moveFocusRight() {
    switch (focusField) {
        case EditField::Back:
            focusField = EditField::Save;
            backWidget.setFocused(false);
            saveWidget.setFocused(true);
            break;
        case EditField::Save:
            focusField = EditField::Back;
            backWidget.setFocused(true);
            saveWidget.setFocused(false);
            break;
        default: break;
    }
}
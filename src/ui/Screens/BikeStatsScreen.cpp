#include "BikeStatsScreen.hpp"

void BikeStatsScreen::handleInput(physIO input) {
    if (!anySelected()) {
        if (input.Up.press) moveFocusUp();
        else if (input.Down.press) moveFocusDown();
        else if (input.Left.press) moveFocusLeft();
        else if (input.Right.press) moveFocusRight();
    } 
    switch (focusField) {
        case EditField::Mass:
            if (bikeMassWidget.isSelected()) {
                if (input.Up.state && bikeMassWidget.shouldRepeat(input.Up.heldTime)) {
                    if (_repeatCount > 10) {
                        _mass += 10;
                    } else {
                        ++_mass;
                    }
                    _repeatCount ++;
                } else if (input.Down.state && bikeMassWidget.shouldRepeat(input.Down.heldTime)) {
                    if (_repeatCount > 10) {
                        _mass -= 10;
                    } else {
                        --_mass;
                    }
                    _repeatCount ++;
                } else if (input.Up.press) ++_mass;
                else if (input.Down.press) --_mass;
            }
            if(input.Select.press) bikeMassWidget.setSelected(!bikeMassWidget.isSelected());
            break;
        case EditField::WheelCircumference:
            if (wheelCircWidget.isSelected()) {
                if (input.Up.state && wheelCircWidget.shouldRepeat(input.Up.heldTime)) {
                    if (_repeatCount > 10) {
                        _circumference += 10;
                    } else {
                        ++_circumference;
                    }
                    _repeatCount ++;
                } else if (input.Down.state && wheelCircWidget.shouldRepeat(input.Down.heldTime)) {
                    if (_repeatCount > 10) {
                        _circumference -= 10;
                    } else {
                        --_circumference;
                    }
                    _repeatCount ++;
                } else if (input.Up.press) ++_circumference;
                else if (input.Down.press) --_circumference;
            }
            if(input.Select.press) wheelCircWidget.setSelected(!wheelCircWidget.isSelected());
            break;
        case EditField::Logger:
            if(input.Select.press) loggerWidget.setSelected(!loggerWidget.isSelected());
            if (loggerWidget.isSelected()) {
                if (input.Up.press) {
                    if (_logger == LoggerType::FIT) _logger = LoggerType::CSV;
                    else if (_logger == LoggerType::TCX) _logger = LoggerType::FIT;
                    else if (_logger == LoggerType::CSV) _logger = LoggerType::TCX;
                } else if (input.Down.press) {
                    if (_logger == LoggerType::FIT) _logger = LoggerType::TCX;
                    else if (_logger == LoggerType::TCX) _logger = LoggerType::CSV;
                    else if (_logger == LoggerType::CSV) _logger = LoggerType::FIT;
                }
            }
            break;

        case EditField::Back:
            backWidget.handleInput(input);
            break;
        case EditField::Save:
            saveWidget.handleInput(input);
            break;
        default: break;
    }
    if (!input.Up.state && !input.Down.state) _repeatCount = 0;
}

void BikeStatsScreen::moveFocusUp() {
    switch (focusField) {
        case EditField::Mass: focusField = EditField::Back; break;
        case EditField::WheelCircumference: focusField = EditField::Mass; break;
        case EditField::Logger: focusField = EditField::WheelCircumference; break;
        case EditField::Back: focusField = EditField::Logger; break;
        case EditField::Save: focusField = EditField::Logger; break;
        default: break;
    }
}

void BikeStatsScreen::moveFocusDown() {
    switch (focusField) {
        case EditField::Mass: focusField = EditField::WheelCircumference; break;
        case EditField::WheelCircumference: focusField = EditField::Logger; break;
        case EditField::Logger: focusField = EditField::Back; break;
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Mass; break;
        default: break;
    }
}

void BikeStatsScreen::moveFocusLeft() {
    switch (focusField) {
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Back; break;
        default: break;
    }
}

void BikeStatsScreen::moveFocusRight() {
    switch (focusField) {
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Back; break;
        default: break;
    }
}
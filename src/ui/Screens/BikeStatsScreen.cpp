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
        case EditField::AutoPause:
            if(input.Select.press) autoPauseWidget.setSelected(!autoPauseWidget.isSelected());
            if (autoPauseWidget.isSelected()) {
                if (input.Up.press) _autoPause = true;
                else if (input.Down.press) _autoPause = false;
            }
            break;
        case EditField::IdleSleep:
            if(input.Select.press) idleSleepWidget.setSelected(!idleSleepWidget.isSelected());
            if (idleSleepWidget.isSelected()) {
                if (input.Up.state && idleSleepWidget.shouldRepeat(input.Up.heldTime)) {
                    if (_repeatCount > 10) {
                        if (_idleSleepMin < 60) _idleSleepMin += 5;
                    } else {
                        if (_idleSleepMin < 60) ++_idleSleepMin;
                    }
                    _repeatCount ++;
                } else if (input.Down.state && idleSleepWidget.shouldRepeat(input.Down.heldTime)) {
                    if (_repeatCount > 10) {
                        if (_idleSleepMin > 5) _idleSleepMin -= 5;
                    } else {
                        if (_idleSleepMin > 1) --_idleSleepMin;
                    }
                    _repeatCount ++;
                } else if (input.Up.press) {
                    if (_idleSleepMin < 60) ++_idleSleepMin;
                } else if (input.Down.press) {
                    if (_idleSleepMin > 1) --_idleSleepMin;
                }
            }
            break;

        case EditField::AutoLapMode:
            if (autoLapModeWidget.isSelected()) {
                // Cycle Off -> Distance -> Time (Up forward, Down backward, wrapping).
                if (input.Up.press) {
                    _autoLapMode = (AutoLapMode)(((uint8_t)_autoLapMode + 1) % 3);
                } else if (input.Down.press) {
                    _autoLapMode = (AutoLapMode)(((uint8_t)_autoLapMode + 2) % 3);
                }
            }
            if (input.Select.press) autoLapModeWidget.setSelected(!autoLapModeWidget.isSelected());
            break;

        case EditField::AutoLapValue:
            if (autoLapValueWidget.isSelected()) {
                const uint8_t maxVal = (_autoLapMode == AutoLapMode::Distance) ? 100 : 60;
                if (input.Up.state && autoLapValueWidget.shouldRepeat(input.Up.heldTime)) {
                    if (_repeatCount > 10) {
                        if (_autoLapValue < maxVal) _autoLapValue = min((int)(_autoLapValue + 5), (int)maxVal);
                    } else {
                        if (_autoLapValue < maxVal) ++_autoLapValue;
                    }
                    _repeatCount ++;
                } else if (input.Down.state && autoLapValueWidget.shouldRepeat(input.Down.heldTime)) {
                    if (_repeatCount > 10) {
                        if (_autoLapValue > 1) _autoLapValue = max((int)(_autoLapValue - 5), 1);
                    } else {
                        if (_autoLapValue > 1) --_autoLapValue;
                    }
                    _repeatCount ++;
                } else if (input.Up.press) {
                    if (_autoLapValue < maxVal) ++_autoLapValue;
                } else if (input.Down.press) {
                    if (_autoLapValue > 1) --_autoLapValue;
                }
            }
            if (input.Select.press) autoLapValueWidget.setSelected(!autoLapValueWidget.isSelected());
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
        case EditField::AutoLapValue: focusField = EditField::AutoLapMode; break;
        case EditField::AutoLapMode: focusField = EditField::IdleSleep; break;
        case EditField::IdleSleep: focusField = EditField::AutoPause; break;
        case EditField::AutoPause: focusField = EditField::Logger; break;
        case EditField::Back: focusField = EditField::AutoLapValue; break;
        case EditField::Save: focusField = EditField::AutoLapValue; break;
        default: break;
    }
}

void BikeStatsScreen::moveFocusDown() {
    switch (focusField) {
        case EditField::Mass: focusField = EditField::WheelCircumference; break;
        case EditField::WheelCircumference: focusField = EditField::Logger; break;
        case EditField::Logger: focusField = EditField::AutoPause; break;
        case EditField::AutoPause: focusField = EditField::IdleSleep; break;
        case EditField::IdleSleep: focusField = EditField::AutoLapMode; break;
        case EditField::AutoLapMode: focusField = EditField::AutoLapValue; break;
        case EditField::AutoLapValue: focusField = EditField::Back; break;
        case EditField::Back: focusField = EditField::Mass; break;
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
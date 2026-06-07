#include "BiometricsScreen.hpp"

void BiometricsScreen::handleInput(physIO input) {
    if (!anySelected()) {
        if (input.Up.press) moveFocusUp();
        else if (input.Down.press) moveFocusDown();
        else if (input.Left.press) moveFocusLeft();
        else if (input.Right.press) moveFocusRight();
    } 
    switch (focusField) {
        case EditField::Birthday: 
            dateWidget.handleInput(input);
            break;
        case EditField::Mass:
            if (massWidget.isSelected()) {
                if (input.Up.press) ++_mass;
                else if (input.Down.press) --_mass;
            }
            if(input.Select.press) massWidget.setSelected(!massWidget.isSelected());
            break;
        case EditField::CaloricProfile:
            if (calorieWidget.isSelected()) {
                if (input.Up.press) ++_caloricProfile;
                else if (input.Down.press) --_caloricProfile;
            }
            if(input.Select.press) calorieWidget.setSelected(!calorieWidget.isSelected());
            break;
        case EditField::Zone1Start:
            if (zone1StartWidget.isSelected()) {
                if (input.Up.state && zone1StartWidget.shouldRepeat(input.Up.heldTime)) {
                    ++_zone1Start;
                } else if (input.Down.state && zone1StartWidget.shouldRepeat(input.Down.heldTime)) {
                    --_zone1Start;
                } else if (input.Up.press) ++_zone1Start;
                else if (input.Down.press) --_zone1Start;
            }
            if(input.Select.press) zone1StartWidget.setSelected(!zone1StartWidget.isSelected());
            break;
        case EditField::Zone2Start:
            if (zone2StartWidget.isSelected()) {
                if (input.Up.state && zone2StartWidget.shouldRepeat(input.Up.heldTime)) {
                    ++_zone2Start;
                } else if (input.Down.state && zone2StartWidget.shouldRepeat(input.Down.heldTime)) {
                    --_zone2Start;
                } else if (input.Up.press) ++_zone2Start;
                else if (input.Down.press) --_zone2Start;
            }
            if(input.Select.press) zone2StartWidget.setSelected(!zone2StartWidget.isSelected());
            break;
        case EditField::Zone3Start:
            if (zone3StartWidget.isSelected()) {
                if (input.Up.state && zone3StartWidget.shouldRepeat(input.Up.heldTime)) {
                    ++_zone3Start;
                } else if (input.Down.state && zone3StartWidget.shouldRepeat(input.Down.heldTime)) {
                    --_zone3Start;
                } else if (input.Up.press) ++_zone3Start;
                else if (input.Down.press) --_zone3Start;
            }
            if(input.Select.press) zone3StartWidget.setSelected(!zone3StartWidget.isSelected());
            break;
        case EditField::Zone4Start:
            if (zone4StartWidget.isSelected()) {
                if (input.Up.state && zone4StartWidget.shouldRepeat(input.Up.heldTime)) {
                    ++_zone4Start;
                } else if (input.Down.state && zone4StartWidget.shouldRepeat(input.Down.heldTime)) {
                    --_zone4Start;
                } else if (input.Up.press) ++_zone4Start;
                else if (input.Down.press) --_zone4Start;
            }
            if(input.Select.press) zone4StartWidget.setSelected(!zone4StartWidget.isSelected());
            break;
        case EditField::Zone5Start:
            if (zone5StartWidget.isSelected()) {
                if (input.Up.state && zone5StartWidget.shouldRepeat(input.Up.heldTime)) {
                    ++_zone5Start;
                } else if (input.Down.state && zone5StartWidget.shouldRepeat(input.Down.heldTime)) {
                    --_zone5Start;
                } else if (input.Up.press) ++_zone5Start;
                else if (input.Down.press) --_zone5Start;
            }
            if(input.Select.press) zone5StartWidget.setSelected(!zone5StartWidget.isSelected());
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

void BiometricsScreen::moveFocusUp() {
    switch (focusField) {
        case EditField::Birthday: focusField = EditField::Back;break;
        case EditField::Mass: focusField = EditField::Birthday; break;
        case EditField::CaloricProfile: focusField = EditField::Mass; break;
        case EditField::Zone1Start: focusField = EditField::CaloricProfile; break;
        case EditField::Zone2Start: focusField = EditField::Zone1Start; break;
        case EditField::Zone3Start: focusField = EditField::Zone2Start; break;
        case EditField::Zone4Start: focusField = EditField::Zone3Start; break;
        case EditField::Zone5Start: focusField = EditField::Zone4Start; break;
        case EditField::Back: focusField = EditField::Zone5Start; break;
        default: break;
    }
}

void BiometricsScreen::moveFocusDown() {
    switch (focusField) {
        case EditField::Birthday: focusField = EditField::Mass;break;
        case EditField::Mass: focusField = EditField::CaloricProfile; break;
        case EditField::CaloricProfile: focusField = EditField::Zone1Start; break;
        case EditField::Zone1Start: focusField = EditField::Zone2Start; break;
        case EditField::Zone2Start: focusField = EditField::Zone3Start; break;
        case EditField::Zone3Start: focusField = EditField::Zone4Start; break;
        case EditField::Zone4Start: focusField = EditField::Zone5Start; break;
        case EditField::Zone5Start: focusField = EditField::Back; break;
        case EditField::Back: focusField = EditField::Birthday; break;
        default: break;
    }
}

void BiometricsScreen::moveFocusLeft() {
    switch (focusField) {
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Back; break;
        default: break;
    }
}

void BiometricsScreen::moveFocusRight() {
    switch (focusField) {
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Back; break;
        default: break;
    }
}
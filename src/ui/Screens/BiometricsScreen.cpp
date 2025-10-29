#include "BiometricsScreen.hpp"

void BiometricsScreen::handleInput(physIO input) {
    if (!anySelected()) {
        if (input.Up.press) moveFocusUp();
        else if (input.Down.press) moveFocusDown();
        else if (input.Left.press) moveFocusLeft();
        else if (input.Right.press) moveFocusRight();
    } else {
        switch (focusField) {
            case EditField::Birthday: 
                dateWidget.handleInput(input);
                break;
            case EditField::Mass:
                if (input.Up.press) ++_mass;
                else if (input.Down.press) --_mass;
                else if (input.Select.press) massWidget.setSelected(!massWidget.isSelected());
                break;
            case EditField::CaloricProfile:
                if (input.Up.press) ++_caloricProfile;
                else if (input.Down.press) --_caloricProfile;
                else if (input.Select.press) calorieWidget.setSelected(!calorieWidget.isSelected());
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

void BiometricsScreen::moveFocusUp() {
    switch (focusField) {
        case EditField::Birthday: focusField = EditField::Back;break;
        case EditField::Mass: focusField = EditField::Birthday; break;
        case EditField::CaloricProfile: focusField = EditField::Mass; break;
        case EditField::Back: focusField = EditField::CaloricProfile; break;
        case EditField::Save: focusField = EditField::CaloricProfile; break;
        default: break;
    }
    dateWidget.setFocused(focusField == EditField::Birthday);
    massWidget.setFocused(focusField == EditField::Mass);
    calorieWidget.setFocused(focusField == EditField::CaloricProfile);
    backWidget.setFocused(focusField == EditField::Back);
    saveWidget.setFocused(focusField == EditField::Save);
}

void BiometricsScreen::moveFocusDown() {
    switch (focusField) {
        case EditField::Birthday: focusField = EditField::Mass;break;
        case EditField::Mass: focusField = EditField::CaloricProfile; break;
        case EditField::CaloricProfile: focusField = EditField::Back; break;
        case EditField::Back: focusField = EditField::Birthday; break;
        case EditField::Save: focusField = EditField::Birthday; break;
        default: break;
    }
    dateWidget.setFocused(focusField == EditField::Birthday);
    massWidget.setFocused(focusField == EditField::Mass);
    calorieWidget.setFocused(focusField == EditField::CaloricProfile);
    backWidget.setFocused(focusField == EditField::Back);
    saveWidget.setFocused(focusField == EditField::Save);
}

void BiometricsScreen::moveFocusLeft() {
    switch (focusField) {
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Back; break;
        default: break;
    }
    dateWidget.setFocused(focusField == EditField::Birthday);
    massWidget.setFocused(focusField == EditField::Mass);
    calorieWidget.setFocused(focusField == EditField::CaloricProfile);
    backWidget.setFocused(focusField == EditField::Back);
    saveWidget.setFocused(focusField == EditField::Save);
}

void BiometricsScreen::moveFocusRight() {
    switch (focusField) {
        case EditField::Back: focusField = EditField::Save; break;
        case EditField::Save: focusField = EditField::Back; break;
        default: break;
    }
    dateWidget.setFocused(focusField == EditField::Birthday);
    massWidget.setFocused(focusField == EditField::Mass);
    calorieWidget.setFocused(focusField == EditField::CaloricProfile);
    backWidget.setFocused(focusField == EditField::Back);
    saveWidget.setFocused(focusField == EditField::Save);
}
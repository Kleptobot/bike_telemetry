#include "GPSScreen.hpp"

void GPSScreen::handleInput(physIO input) {
    if (!anySelected()) {
        if (input.Up.press) moveFocusUp();
        else if (input.Down.press) moveFocusDown();
        else if (input.Left.press) moveFocusLeft();
        else if (input.Right.press) moveFocusRight();
        else if (input.Select.press) {
            switch (focusField) {
                case EditField::GGArate:
                case EditField::GLLrate:
                case EditField::GSArate:
                case EditField::GSVrate:
                case EditField::RMCrate:
                case EditField::VTGrate:
                case EditField::ZDArate:
                case EditField::GRSrate:
                case EditField::GSTrate:
                case EditField::GNSrate:
                    ratesWidgetRefs[(int)focusField-2].get().setSelected(true);
                    break;
                default: break;
            }
        }
    } else {
        switch (focusField) {
            case EditField::GGArate:
            case EditField::GLLrate:
            case EditField::GSArate:
            case EditField::GSVrate:
            case EditField::RMCrate:
            case EditField::VTGrate:
            case EditField::ZDArate:
            case EditField::GRSrate:
            case EditField::GSTrate:
            case EditField::GNSrate:
                    if (input.Up.state && ratesWidgetRefs[(int)focusField-2].get().shouldRepeat(input.Up.heldTime)) {
                        ++_rates[(int)focusField-2];
                    } else if (input.Down.state && ratesWidgetRefs[(int)focusField-2].get().shouldRepeat(input.Down.heldTime)) {
                        --_rates[(int)focusField-2];
                    } else if (input.Up.press) ++_rates[(int)focusField-2];
                    else if (input.Down.press) --_rates[(int)focusField-2];
                    if(input.Select.press) 
                        ratesWidgetRefs[(int)focusField-2].get().setSelected(false);
                break;
            default: break;
        }
    }

    restGPS.handleInput(input);
    restoreGPSDefaults.handleInput(input);
    saveNVRAM.handleInput(input);
    backWidget.handleInput(input);
    saveWidget.handleInput(input);
}

void GPSScreen::update(float dt) {
    restGPS.setFocused(focusField == EditField::resetGPS);
    restoreGPSDefaults.setFocused(focusField == EditField::restoreDefaults);
    saveNVRAM.setFocused(focusField == EditField::saveNVRAM);
    backWidget.setFocused(focusField == EditField::back);
    saveWidget.setFocused(focusField == EditField::save);
    for(int i = 0; i < 10; ++i) {
        ratesWidgetRefs[i].get().setFocused(focusField == EditField(int(EditField::GGArate) + i));
        ratesWidgetRefs[i].get().setText(String(_rates[i]));

        //on the falling edge of selected, emit an event to the app to update the rate
        if (!ratesWidgetRefs[i].get().isSelected() && prevSelectedRates[i]) {
            AppEvent e;
            e.type = AppEventType::setGPSNMEARate;
            e.payload = NMEArateChange{i, _rates[i]};
            emitAppEvent(e);
        }
        prevSelectedRates[i] = ratesWidgetRefs[i].get().isSelected();
    }
    
}
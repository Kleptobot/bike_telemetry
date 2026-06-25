#include "TimeWidget.hpp"
#include "Display/Display.hpp"
#include "DebugConfig.hpp"

void TimeWidget::handleInput(physIO input) {

    if (input.Select.press) {
        selected = !selected;
    }

    if (selected) {
        // Handle Up: press or held with repeat
        if (input.Up.state && shouldRepeat(input.Up.heldTime)) {
            editField(focusField,1);
        }else if (input.Up.press) {
            editField(focusField,1);
        } 

        // Handle Down: press or held with repeat
         if (input.Down.state && shouldRepeat(input.Down.heldTime)) {
            editField(focusField,-1);
        } else if (input.Down.press) {
            editField(focusField,-1);
        }
    } else {
        // Handle Left: press only (no repeat for focus change)
        if (input.Left.press)  moveFocusLeft();
        
        // Handle Right: press only (no repeat for focus change)
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

    auto lt = _date.local();

    if(lt.hour < 10) hourText.setText("0"+String(lt.hour));
    else hourText.setText(String(lt.hour));

    if(lt.minute < 10) minuteText.setText("0"+String(lt.minute));
    else minuteText.setText(String(lt.minute));

    if(lt.second < 10) secondText.setText("0"+String(lt.second));
    else secondText.setText(String(lt.second));
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

void TimeWidget::editField(EditField field, int val) {
    switch (field) {
        case EditField::Hour:
            _date.add_hours(val);
            hourText.setText(String( _date.hour()));
            break;

        case EditField::Minute:
            _date.add_minutes(val);
            minuteText.setText(String( _date.minute()));
            break;

        case EditField::Second:
            _date.add_seconds(val);
            secondText.setText(String( _date.second()));
            break;

        default: break;
    }
}

void TimeWidget::render() {
    if (!visible){
        Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
        return;
    }

    // draw them
    hourText.render();
    Disp::print(":");
    minuteText.render();
    Disp::print(":");
    secondText.render();
}

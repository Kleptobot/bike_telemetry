#include "DateWidget.hpp"
#include "Display/Display.hpp"

void DateWidget::handleInput(physIO input) {

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

void DateWidget::update(float dt) {
    dayText.setFocused(focusField == EditField::Day && focused);
    monthText.setFocused(focusField == EditField::Month && focused);
    yearText.setFocused(focusField == EditField::Year && focused);

    dayText.setSelected(selected && focusField == EditField::Day);
    monthText.setSelected(selected && focusField == EditField::Month);
    yearText.setSelected(selected && focusField == EditField::Year);
    
    auto lt = _date.local();

    if(lt.day < 10) dayText.setText("0"+String(lt.day));
    else dayText.setText(String(lt.day));

    if(lt.month < 10) monthText.setText("0"+String(lt.month));
    else monthText.setText(String(lt.month));

    if(lt.year < 10) yearText.setText("0"+String(lt.year));
    else yearText.setText(String(lt.year));
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

void DateWidget::editField(EditField field, int val) {
    switch (field) {
        case EditField::Day:
            _date.add_days(val);
            dayText.setText(String( _date.day()));
            break;

        case EditField::Month:
            _date.add_months(val);
            monthText.setText(String( _date.month()));
            break;

        case EditField::Year:
            _date.add_years(val);
            yearText.setText(String( _date.year()));
            break;

        default: break;
    }
}

void DateWidget::render() {
    if (!visible) return;
    // update field visuals

    // draw them
    dayText.render();
    Disp::print("/");
    monthText.render();
    Disp::print("/");
    yearText.render();
}

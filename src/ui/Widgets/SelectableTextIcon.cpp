#include "SelectableTextIcon.hpp"
#include "Display/Display.hpp"

void SelectableTextIconWidget::render() {
    if (!visible) return;

    Disp::setCursor(x, y);
    Disp::setTextSize(2);

    int16_t x1, y1;
    uint16_t w, h;
    Disp::getTextBounds(text, x + 16, y, &x1, &y1, &w, &h);

    if (selected) {
        Disp::fillRect(x - 1, y - 1, w+17, h+1, DispCol::WHITE);
        Disp::setTextColor(DispCol::BLACK);
    } else if (focused) {
        Disp::drawRect(x - 2, y - 2, w + 18, h + 2, DispCol::WHITE);
        Disp::setTextColor(DispCol::WHITE);
    } else {
        Disp::setTextColor(DispCol::WHITE);
    }
    
    Disp::drawBitmap(x, y, bitmap, 16, 16, DispCol::WHITE);
    Disp::setCursor(x + 16, y);
    Disp::print(text);
}

void SelectableTextIconWidget::handleInput(physIO input) {
    if (input.Select.press && focused && onPress) {
        onPress();  // fire callback
    }
}
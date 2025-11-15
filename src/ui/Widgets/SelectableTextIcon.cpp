#include "SelectableTextIcon.hpp"
#include "Display/Display.hpp"

void SelectableTextIconWidget::render() {
    if (!visible) return;

    Disp::setCursor(x, y);
    Disp::setTextSize(_text_size);

    int16_t x1, y1;
    uint16_t w, h;
    Disp::getTextBounds(text, x + _icon_height, y, &x1, &y1, &w, &h);

    if (selected) {
        Disp::fillRect(x - 1, y - 1, w+_icon_height + 1, h*2+1, ST77XX_WHITE);
        Disp::setTextColor(ST77XX_BLACK);
    } else if (focused) {
        Disp::drawRect(x - 2, y - 2, w + _icon_height + 4, h*2 + 2, ST77XX_WHITE);
        Disp::setTextColor(ST77XX_WHITE);
    } else {
        Disp::drawRect(x - 2, y - 2, w + _icon_height + 4, h*2 + 2, ST77XX_BLACK);
        Disp::setTextColor(ST77XX_WHITE);
    }
    
    Disp::drawBitmap(x, y, bitmap, _icon_height, _icon_height, ST77XX_WHITE);
    Disp::setCursor(x + _icon_height, y);
    Disp::print(text);
}

void SelectableTextIconWidget::handleInput(physIO input) {
    if (input.Select.press && focused && _onPress) {
        _onPress();  // fire callback
    } else if (input.Select.press && focused) {
        selected = !selected;
    }
}
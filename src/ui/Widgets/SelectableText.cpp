#include "SelectableText.hpp"
#include "Display/Display.hpp"

void SelectableTextWidget::render() {
    if (!visible) return;

    Disp::setCursor(x, y);
    Disp::setTextSize(_text_size);

    int16_t x1, y1;
    uint16_t w, h;
    Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);

    Disp::setTextColor(ST77XX_WHITE);
    if (selected) {
        Disp::fillRect(x - 2, y - 2, w+4, h+4, ST77XX_WHITE);
        Disp::setTextColor(ST77XX_BLACK,ST77XX_BLACK);
    } else if (focused) {
        Disp::drawRect(x - 2, y - 2, w+4, h+4, ST77XX_WHITE);
    }else{
        Disp::drawRect(x - 2, y - 2, w+4, h+4, ST77XX_BLACK);
    }

    Disp::print(text);
}

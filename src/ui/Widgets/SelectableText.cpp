#include "SelectableText.hpp"
#include "Display/Display.hpp"

void SelectableTextWidget::render() {
    if (!visible) return;

    Disp::setCursor(_x, _y);
    Disp::setTextSize(_text_size);

    int16_t x1, y1;
    uint16_t w, h;
    Disp::getTextBounds(text, _x, _y, &x1, &y1, &w, &h);

    Disp::setTextColor(ST77XX_WHITE);
    if (selected) {
        Disp::fillRect(_x - 2, _y - 2, w+4, h+4, ST77XX_WHITE);
        Disp::setTextColor(ST77XX_BLACK,ST77XX_BLACK);
    } else if (focused) {
        Disp::drawRect(_x - 2, _y - 2, w+4, h+4, ST77XX_WHITE);
    }
    // Unfocused: no border. The canvas is cleared and fully re-rendered every
    // frame, so nothing needs to be erased here - drawing a black border is
    // what previously left stale border pixels on the display when focus was
    // lost and the dirty rect didn't cover the old border exactly.

    Disp::print(text);
}

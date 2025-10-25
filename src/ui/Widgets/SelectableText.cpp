// UI/SelectableTextWidget.cpp
#include "SelectableText.hpp"
#include "Display/Display.hpp"

void SelectableTextWidget::render() {
    if (!visible) return;

    Disp::setCursor(x, y);
    Disp::setTextSize(2);

    int boxWidth = text.length() * 12;
    int boxHeight = 18;

    if (selected) {
        Disp::fillRect(x - 2, y - 2, boxWidth, boxHeight, DispCol::WHITE);
        Disp::setTextColor(DispCol::BLACK);
    } else if (focused) {
        Disp::drawRect(x - 2, y - 2, boxWidth, boxHeight, DispCol::WHITE);
        Disp::setTextColor(DispCol::WHITE);
    } else {
        Disp::setTextColor(DispCol::WHITE);
    }

    Disp::print(text);
}

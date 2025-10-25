#pragma once
#include <Adafruit_GFX.h>
#include "UIElement.hpp"

class UILabel : public UIElement {
public:
    UILabel(const String& text, int x, int y)
        : text(text), x(x), y(y) {}

    void setText(const String& t) { text = t; }
    void render(int x, int y) override {
        Disp::drawText(x,y,text,DisplayColor::WHITE);
    }

    static void attachDisplay(Adafruit_GFX* gfx) { display = gfx; }

private:
    String text;
    int x, y;
    static Adafruit_GFX* display;
};

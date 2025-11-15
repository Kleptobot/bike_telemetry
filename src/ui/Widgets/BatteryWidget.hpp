#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"

class BatteryWidget : public Widget {
    public:
        BatteryWidget(int x, int y)
            : Widget(x, y, 32, 16) {}

        void render() override {
            if (!visible) {
                Disp::fillRect(x,y,width,height,ST77XX_BLACK);
                return;
            }
            Disp::setTextSize(1);
            Disp::drawBitmap(x, y, epd_bitmap_battery, 32, 16, ST77XX_WHITE);
            Disp::setCursor(x+7, y+5);
            Disp::print(_batt);
        }

        void setBat(uint8_t batt) {
            _batt = batt;
        }

    private:
        uint8_t _batt;
};
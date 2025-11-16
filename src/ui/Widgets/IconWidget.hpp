#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"

class IconWidget : public Widget {
    public:
        IconWidget(int x, int y, int w, int h, const uint8_t* icon)
            : Widget(x, y, w, h),
            _icon(icon) {}
        

        void render() override {
            if (!visible) {
                Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
                return;
            } 
            Disp::drawBitmap(x,y,_icon,_width,_height,ST77XX_WHITE);
        }

        void setIcon(const uint8_t* icon) {
            _icon = icon;
            invalidate();
        }

    private:
        const uint8_t* _icon;
};
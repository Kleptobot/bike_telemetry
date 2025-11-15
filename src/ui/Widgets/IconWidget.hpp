#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"

class IconWidget : public Widget {
    public:
        IconWidget(int x, int y, int w, int h, const uint8_t* icon)
            : Widget(x, y, w, h),
            _icon(icon) {}
        

        void render() override {
            if (!visible && visible_last) {
                Disp::fillRect(x,y,width,height,ST77XX_BLACK);
            } 
            else if(visible) {
                Disp::drawBitmap(x,y,_icon,width,height,ST77XX_WHITE);
            }
            visible_last = visible;
        }

    private:
        const uint8_t* _icon;
};
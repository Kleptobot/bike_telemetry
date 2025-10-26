#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"

class IconWidget : public Widget {
    public:
        IconWidget(int x, int y, int w, int h, const uint8_t* icon)
            : Widget(x, y, w, h),
            _icon(icon) {}
        

        void render() override {
            if (!visible) return;
            Disp::drawBitmap(x,y,_icon,width,height,DispCol::WHITE);
        }

    private:
        const uint8_t* _icon;
};
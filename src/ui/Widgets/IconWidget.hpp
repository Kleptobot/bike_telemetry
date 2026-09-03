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
                // No explicit erase: UIManager::render clears the whole canvas
                // every frame, and a black fill here would punch a hole in any
                // graphic drawn beneath this slot (e.g. the main-screen switch
                // cross behind the hidden stop icon).
                return;
            } 
            Disp::drawBitmap(_x,_y,_icon,_width,_height,ST77XX_WHITE);
        }

        void setIcon(const uint8_t* icon) {
            if (_icon != icon) invalidate();
            _icon = icon;
        }

    private:
        const uint8_t* _icon;
};
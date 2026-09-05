#pragma once
#include <Arduino.h>
#include "DebugConfig.hpp"
#include "UI/Widgets/Widget.hpp"
#include "display/Display.hpp"

class SelectableTextWidget : public Widget {
public:
    SelectableTextWidget(int x, int y, const String& text, uint8_t text_size=2 )
        : Widget(x, y), text(text), _text_size(text_size) {
        _height = 8*_text_size;
        _width = 6*_text_size*text.length();
        }

    void setText(const String& t) {
        if (t==text) return;
        // Mark the OLD footprint dirty before mutating, so any area the widget
        // shrinks out of gets repainted (avoids stale pixels on the display).
        invalidate();
        text = t;
        int16_t x1,y1;
        uint16_t w,h;
        // getTextBounds uses the canvas' current text size, which may have been
        // left at a different value by another widget's render - set ours first.
        Disp::setTextSize(_text_size);
        Disp::getTextBounds(text, _x, _y, &x1, &y1, &w, &h);
        _width = w;
        _height = h;
        invalidate();
    }
    void setSize(uint8_t size) { 
        // Mark the OLD footprint dirty before mutating.
        invalidate();
        _text_size = size;
        int16_t x1,y1;
        uint16_t w,h;
        Disp::setTextSize(_text_size);
        Disp::getTextBounds(text, _x, _y, &x1, &y1, &w, &h);
        _width = w;
        _height = h;
        invalidate();
    }
        
    void invalidate() override {
        // Border is drawn 2px outside the text bounds on all sides.
        Disp::markDirty(max(_x-2,0), max(_y-2,0), width()+4, height()+4);
        if (ENABLE_INVALIDATE_DEBUG) {
            Serial.println("[Widget] Invalidated area: (" + String(_x) + "," + String(_y) + "," + String(width()) + "," + String(height()) + ")");
        }
    }

    void render() override;

private:
    String text;
    uint8_t _text_size = 2;

};
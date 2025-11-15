#pragma once
#include <Arduino.h>
#include "UI/Widgets/Widget.hpp"
#include "display/Display.hpp"

class SelectableTextWidget : public Widget {
public:
    SelectableTextWidget(int x, int y, const String& text, uint8_t text_size=2 )
        : Widget(x, y), text(text), _text_size(text_size) {}

    void setText(const String& t) {
        if(t.length() > text.length()) 
            invalidate();
        text = t; 
        int16_t x1,y1;
        uint16_t w,h;
        Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);
        _width = w;
        _height = h;
        invalidate();
    }
    void setSize(uint8_t size) { 
        if(size > _text_size)
            invalidate();
        _text_size = size;
        int16_t x1,y1;
        uint16_t w,h;
        Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);
        _width = w;
        _height = h;
        invalidate();
    }
        
    void invalidate() override {
        Disp::markDirty(x-2, y-2, width()+4, height()+4);
    }

    void render() override;

private:
    String text;
    uint8_t _text_size = 2;

};
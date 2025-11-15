#pragma once
#include <Arduino.h>
#include "UI/Widgets/Widget.hpp"
#include "display/Display.hpp"

class SelectableTextWidget : public Widget {
public:
    SelectableTextWidget(int x, int y, const String& text, uint8_t text_size=2 )
        : Widget(x, y), text(text), _text_size(text_size) {}

    void setText(const String& t) { 
        text = t; 
        int16_t x1,y1;
        uint16_t w,h;
        Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);
        width = w;
    }
    void setSize(uint8_t size) { 
        _text_size = size;
        height = 8*_text_size;
        int16_t x1,y1;
        uint16_t w,h;
        Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);
        width = w;
    }
    void setFocused(bool f) { focused = f; }
    void setSelected(bool s) { selected = s; }

    bool isFocused() const { return focused; }
    bool isSelected() const { return selected; }

    void render() override;

protected:
    bool focused = false;
    bool selected = false;

private:
    String text;
    uint8_t _text_size = 2;

};
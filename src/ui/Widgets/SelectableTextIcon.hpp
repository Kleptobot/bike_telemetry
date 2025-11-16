#pragma once
#include <Arduino.h>
#include "UI/Widgets/Widget.hpp"
#include "display/Display.hpp"

struct IconText {
    const uint8_t* bitmap;
    String text;
};

class SelectableTextIconWidget : public Widget {
public:

    SelectableTextIconWidget(int x, int y, const String& text, const uint8_t* bitmap, uint8_t text_size=2, uint8_t icon_height=16)
        : Widget(x, y), 
        text(text) , 
        bitmap(bitmap),
        _text_size(text_size),
        _icon_height(icon_height){
        _height = max(_icon_height,8*_text_size);
        _width = _icon_height+6*_text_size*text.length();
        }

    void invalidate() override {
        int x_safe = max(x-2,0);
        int y_safe = max(y-2,0);
        Disp::markDirty(x_safe, y_safe, width()+4, height()+4);
    }

    void setText(const String& t) { 
        if(t.length() < text.length()) 
            invalidate();
        text = t;
        _height = max(_icon_height,8*_text_size);
        _width = _icon_height+6*_text_size*text.length();
        invalidate();
    }
    void setSize(uint8_t size) { 
        if(size < _text_size)
            invalidate();
        _text_size = size;
        _height = max(_icon_height,8*_text_size);
        _width = _icon_height+6*_text_size*text.length();
        invalidate();
    }

    void setColor(uint16_t color) { _color = color; }

    void render() override;
    void render(int x, int y) override {
        bool invalidateAftermove = (x != this->x) || (y != this->y);
        this->x = x;
        this->y = y;
        if(invalidateAftermove) invalidate();
        render();
    }
    void handleInput(physIO input) override;

    //callback for press events
    using Callback = std::function<void()>;
    virtual void setOnPress(Callback cb) { _onPress = cb; }

private:
    String text;
    const uint8_t* bitmap;
    uint8_t _text_size = 2, _icon_height = 16;
    uint16_t _color = ST77XX_WHITE;
    Callback _onPress = nullptr;
};
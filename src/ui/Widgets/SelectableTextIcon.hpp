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
            _height = _icon_height;
            int16_t x1,y1;
            uint16_t w,h;
            Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);
            _width = icon_height+w;
        }

    void invalidate() override {
        Disp::markDirty(x-2, y-2, width()+4+_icon_height, height()+4);
    }

    void setText(const String& t) { 
        text = t;
        int16_t x1,y1;
        uint16_t w,h;
        Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);
        _width = _icon_height+w;
        invalidate();
    }
    void setSize(uint8_t size) { 
        _text_size = size;
        _height = _icon_height;
        int16_t x1,y1;
        uint16_t w,h;
        Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);
        _width = _icon_height+w;
        invalidate();
    }
    void setColor(uint16_t color) { _color = color; }

    void render() override;
    void render(int x, int y) override {
        this->x = x;
        this->y = y;
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
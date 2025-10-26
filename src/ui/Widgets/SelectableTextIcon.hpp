#pragma once
#include <Arduino.h>
#include "UI/Widgets/Widget.hpp"

struct IconText {
    const uint8_t* bitmap;
    String text;
};

class SelectableTextIconWidget : public Widget {
public:

    SelectableTextIconWidget(int x, int y, const String& text, const uint8_t* bitmap)
        : Widget(x, y), text(text) , bitmap(bitmap){}

    void setText(const String& t) { text = t; }
    void setFocused(bool f) { focused = f; }
    void setSelected(bool s) { selected = s; }

    bool isFocused() const { return focused; }
    bool isSelected() const { return selected; }

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
    bool focused = false;
    bool selected = false;
    Callback _onPress = nullptr;
};
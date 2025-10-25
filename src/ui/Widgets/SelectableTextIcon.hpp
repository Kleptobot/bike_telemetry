#pragma once
#include <Arduino.h>
#include <functional>
#include "UI/Widgets/Widget.hpp"

class SelectableTextIconWidget : public Widget {
public:
    using Callback = std::function<void()>;

    SelectableTextIconWidget(int x, int y, const String& text, const uint8_t* bitmap)
        : Widget(x, y), text(text) , bitmap(bitmap){}

    void setText(const String& t) { text = t; }
    void setFocused(bool f) { focused = f; }
    void setSelected(bool s) { selected = s; }

    bool isFocused() const { return focused; }
    bool isSelected() const { return selected; }

    void setOnPress(Callback cb) { onPress = cb; }

    void render() override;
    void handleInput(physIO input) override;

private:
    String text;
    const uint8_t* bitmap;
    bool focused = false;
    bool selected = false;
    Callback onPress = nullptr;
};
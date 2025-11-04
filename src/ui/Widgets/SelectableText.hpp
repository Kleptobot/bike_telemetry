#pragma once
#include "UI/Widgets/Widget.hpp"
#include <Arduino.h>

class SelectableTextWidget : public Widget {
public:
    SelectableTextWidget(int x, int y, const String& text)
        : Widget(x, y), text(text) {}

    void setText(const String& t) { text = t; }
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
};
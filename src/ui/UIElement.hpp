#pragma once
#include <Arduino.h>
#include "HAL/InputInterface.hpp"
#include "display/Display.hpp"   // include the DisplayUtil and DisplayColor types

class UIElement {
public:
    virtual ~UIElement() = default;
    virtual void update(float dt) {}
    virtual void render(int x, int y) = 0;

    // Optional focus/navigation for UI widgets
    virtual void focus(bool focused) { this->focused = focused; }
    virtual bool isFocusable() const { return false; }
    virtual void select(bool focused) { this->selected = selected; }
    virtual bool isSelectable() const { return false; }
    
    virtual void handleInput(const physIO input) {}
    virtual int getHeight() const { return 0; }
    virtual int getWidth() const { return 0; }

protected:
    bool focused = false;
    bool selected = false;
};

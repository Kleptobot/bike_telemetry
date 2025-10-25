// UI/Widget.h
#pragma once
#include "HAL/InputInterface.hpp"   // your struct for button state, etc.
#include <stdint.h>

class Widget {
public:
    Widget(int x = 0, int y = 0, int w = 0, int h = 0)
        : x(x), y(y), width(w), height(h) {}

    virtual ~Widget() = default;

    // Core lifecycle methods
    virtual void render() {};
    virtual void handleInput(physIO input) {} // optional override

    // Positioning
    void setPosition(int nx, int ny) { x = nx; y = ny; }
    void move(int dx, int dy) { x += dx; y += dy; }
    int getX() const { return x; }
    int getY() const { return y; }

    // Sizing
    void setSize(int w, int h) { width = w; height = h; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Visibility
    void show() { visible = true; }
    void hide() { visible = false; }
    bool isVisible() const { return visible; }

    //interaction
    void setFocused(bool f) { focused = f; }
    bool isFocused() const { return focused; }

    void setSelected(bool s) { selected = s; }
    bool isSelected() const { return selected; }

protected:
    int x, y;
    int width, height;
    bool visible = true;
    bool focused = false;
    bool selected = false;
};
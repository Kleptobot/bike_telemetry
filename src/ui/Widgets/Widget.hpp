// UI/Widget.h
#pragma once
#include "HAL/InputInterface.hpp"
#include "display/Display.hpp"
#include <stdint.h>
#include <functional>


class Widget {
public:
    Widget(int x = 0, int y = 0, int w = 0, int h = 0)
        : x(x), y(y), _width(w), _height(h) {}

    virtual ~Widget() = default;

    // Core lifecycle methods
    virtual void render() {};
    virtual void render(int x, int y) {};
    virtual void update(float dt) {};
    virtual void handleInput(physIO input) {} // optional override

    // Positioning
    void setPosition(int nx, int ny) { 
       if((x != nx) || (y != ny)) invalidate();
        x = nx; 
        y = ny; 
    }
    void move(int dx, int dy) { x += dx; y += dy; }
    int getX() const { return x; }
    int getY() const { return y; }

    virtual void invalidate() {
        Disp::markDirty(x, y, width(), height());
    }

    // Sizing
    void setSize(int w, int h) { _width = w; _height = h; }

    // Visibility
    void setVisible(bool newVis) {
        if (visible != newVis) invalidate();
        visible = newVis;
    }
    bool isVisible() const { return visible; }

    //interaction
    virtual void setFocused(bool f) {
        if (focused != f) invalidate();
        focused = f;
    }
    bool isFocused() const {return focused; }

    void setSelected(bool s) {
        if (s != selected) invalidate();
        selected = s;
    }
    virtual bool isSelected() const { return selected; }

    virtual int width() const { return _width; }
    virtual int height() const { return _height; }

protected:
    int x, y;
    int _width, _height;
    bool visible = true;
    bool focused = false;
    bool selected = false;
};
// UI/Widget.h
#pragma once
#include "HAL/InputInterface.hpp"
#include "display/Display.hpp"
#include "DebugConfig.hpp"
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
        if (ENABLE_INVALIDATE_DEBUG) {
            Serial.println("[Widget] Invalidated area: (" + String(x) + "," + String(y) + "," + String(width()) + "," + String(height()) + ")");
        }
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
    
    bool shouldRepeat(uint32_t heldTime) {
        // Start repeating after initial delay
        if (heldTime < lastHeldTime) {
            // This can happen if heldTime resets (e.g., on button release)
            lastHeldTime = 0; // Reset lastHeldTime to avoid issues on next press
        }
        if (heldTime < HOLD_REPEAT_DELAY) return false;
        
        // Check if enough heldTime has passed since last repeat action
        if (heldTime - lastHeldTime < HOLD_REPEAT_INTERVAL) {
            return false;
        }
        lastHeldTime = heldTime; // Update lastHeldTime for next repeat check
        return true;
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

private:
    // Hold press repeat timing
    static constexpr uint32_t HOLD_REPEAT_DELAY = 400;    // ms before repeat starts
    static constexpr uint32_t HOLD_REPEAT_INTERVAL = 100;  // ms between repeats
    uint32_t lastHeldTime = 0;  // Track heldTime of last repeat action
};
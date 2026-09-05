#pragma once
#include "Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"

class CursorWidget : public Widget {
public:
    CursorWidget(int cx, int cy, int w, int h) :
    _cx(cx),
    _cy(cy)
    {
        setSize(w, h);
    }

    void handleInput(physIO input) override {
        if (input.Select.press) setSelected(!isSelected());
    }

    void setPosition(int cx, int cy) {
        if (cx != _cx || cy != _cy) {
            invalidate();          // old rect
            _cx = cx;
            _cy = cy;
            _x = _cx - _hx;
            _y = _cy - _hy;
            invalidate();          // new rect
        }
    }

    void setSize(int w, int h) {
        if (w != _width || h != _height) {
            invalidate();          // old rect
            _width = w;
            _height = h;
            _hx = _width/2;
            _hy = _height/2;
            _x = _cx - _hx;
            _y = _cy - _hy;
            invalidate();          // new rect
        }
    }

    // The 3px outline extends 1px beyond the widget rect on all sides, so
    // dirty-rect marking must cover that ring or moving leaves ghost pixels.
    void invalidate() override {
        Disp::markDirty(_x - 1, _y - 1, width() + 2, height() + 2);
    }

    void render() {
        if (!visible) return;
        // 3px thick outline: nested rects 1px apart around the cell.
        Disp::drawRect(_x - 1, _y - 1, _width + 2, _height + 2, ST77XX_ORANGE);
        Disp::drawRect(_x,     _y,     _width,     _height,     ST77XX_ORANGE);
        Disp::drawRect(_x + 1, _y + 1, _width - 2, _height - 2, ST77XX_ORANGE);
    }

    const int& CX() const { return _cx; }
    const int& CY() const { return _cy; }

    void update(float dt) override {
    }

private:
    int _cx;
    int _cy;
    int _hx;
    int _hy;


};
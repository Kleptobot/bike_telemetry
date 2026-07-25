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
        setSize(h,w);
    }

    void handleInput(physIO input) override {
        if (input.Select.press) setSelected(!isSelected());
    }

    void setPosition(int cx, int cy) {
        if ( (cx != _cx) || (cy != _cy)) invalidate();
        _cx = cx;
        _cy = cy;
        _x = _width - _hx;
        _y = _height - _hy;
    }

    void setSize(int w, int h) {
        if ( (w != _width) || (h != _height)) invalidate();
        _width = w;
        _height = h;
        _hx = _width/2;
        _hy = _height/2;
        setPosition(_cx, _cy);
    }

    void render() {
        if (!visible) {
            Disp::drawRect(_x , _y , _x + _width, _y + _height , ST77XX_BLACK);
            return;
        }

        Disp::drawRect(_x , _y , _x + _width, _y + _height , ST77XX_WHITE);
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
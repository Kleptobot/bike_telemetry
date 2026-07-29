#pragma once

#include "Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"

class GridOverlayWidget : public Widget {
public:
    GridOverlayWidget(int y, int rows=2, int cols=2)
        : Widget(0, y, 240, 240),
        _rows(rows),
        _cols(cols) {}

    void render() override {
        if (!visible) {
            Disp::fillRect(_x,_y,_width,_height,ST77XX_BLACK);
            return;
        }

        for (int x = 0; x <= _cols; x++ ) {
            Disp::drawLine(x*_colPitch, _y, x*_colPitch, _y + _height - 1, ST77XX_WHITE);
        }
        for (int y = 0; y <= _rows; y++) {
            Disp::drawLine(_x, y*_rowPitch + _y, _x + _width - 1, y*_rowPitch+_y, ST77XX_WHITE);
        }
    }

    void update() {
        _colPitch = _width / _cols;
        _rowPitch = _height / _rows;
    }

    void setRows(int rows) { 
        if (_rows != rows) { invalidate(); _rows = rows; }
    }

    void setCols(int cols) {
        if (_cols != cols) { invalidate(); _cols = cols; }
    }

    const int& colPitch() { return _colPitch; }
    const int& rowPitch() { return _rowPitch; }

private:
    uint8_t _rows = 2, _cols = 2;
    int _colPitch;
    int _rowPitch;
};
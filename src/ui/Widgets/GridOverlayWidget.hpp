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
            // The pitch divides the widget size exactly, so the outermost
            // line would land one pixel off-screen - clamp it inside.
            int lx = _x + min(x * _colPitch, _width - 1);
            Disp::drawLine(lx, _y, lx, gridBottom(), ST77XX_WHITE);
        }
        for (int y = 0; y <= _rows; y++) {
            int ly = _y + min(y * _rowPitch, _height - 1);
            Disp::drawLine(_x, ly, gridRight(), ly, ST77XX_WHITE);
        }
    }

    // Draws the focus highlight for the outer frame. Called by the screen
    // AFTER the cell widgets have rendered, because a tile sharing an edge
    // with the grid repaints the border with its own white frame and would
    // otherwise hide the highlight drawn in render().
    void renderBorder() {
        if (!visible || !focused) return;

        int r = gridRight(), b = gridBottom();
        Disp::drawLine(_x, _y, _x, b, ST77XX_GREEN);        // left
        Disp::drawLine(r, _y, r, b, ST77XX_GREEN);          // right
        Disp::drawLine(_x, _y, r, _y, ST77XX_GREEN);        // top
        Disp::drawLine(_x, b, r, b, ST77XX_GREEN);          // bottom
    }

    using Widget::update;   // see BigDataWidget: avoids hiding update(float)

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
    // Right/bottom edge of the grid in screen coordinates. The pitch divides
    // the widget size exactly, so the raw edge would be one pixel off-screen;
    // clamp it inside so the border is actually visible.
    int gridRight() const { return _x + min(_cols * _colPitch, _width) - 1; }
    int gridBottom() const { return _y + min(_rows * _rowPitch, _height) - 1; }

    uint8_t _rows = 2, _cols = 2;
    int _colPitch;
    int _rowPitch;
};
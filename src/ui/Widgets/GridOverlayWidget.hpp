#pragma once

#include "Widget.hpp"
#include "CursorWidget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"

class GridOverlayWidget : public Widget {
public:
    GridOverlayWidget(int y, int rows=2, int cols=2)
        : Widget(0, y, 240, 240),
        cursor(0,0,120,120),
        _rows(rows),
        _cols(cols) {}

    void render() override {
        if (!visible) {
            Disp::fillRect(_x,_y,_width,_height,ST77XX_BLACK);
            return;
        }

        //draw vertical lines
        for (int x = 0; x <= _cols; x++ ) {
            Disp::drawLine(x*_colPitch, _y, x*_colPitch, _y + _height - 1, ST77XX_WHITE);
        }

        // 2. Draw horizontal lines
        for (int y = 0; y <= _rows; y++) {
            Disp::drawLine(_x, y*_rowPitch + _y, _x + _width - 1, y*_rowPitch+_y, ST77XX_WHITE);
        }

        cursor.render();
    }

    void update() {
        _colPitch = (_width)/_cols;
        _rowPitch = (_height)/_rows;

        hx = _colPitch/2;
        hy = _rowPitch/2;

        cursor.setSize(_colPitch, _rowPitch);
        setCursorPos();
    }

    void setRows(int rows) { 
        if (_rows != rows) {
            invalidate();
            _rows = rows;
            _cursorY = constrain(_cursorY, 0, _rows-1);
            setCursorPos();
        }
    }

    void setCols(int cols) {
        if (_cols != cols) {
            invalidate();
            _cols = cols;
            _cursorX = constrain(_cursorX, 0, _cols-1);
            setCursorPos();
        }
    }

    bool cursorUp() {
        if (_cursorY > 0) {
            _cursorY--;
            setCursorPos();
            invalidate();
            return true;
        }
        return false;
    }

    bool cursorDown() {
        if (_cursorY < (_rows-1)) {
            _cursorY++;
            setCursorPos();
            invalidate();
            return true;
        }
        return false;
    }

    void cursorLeft() {
        if (_cursorX > 0) {
            _cursorX--;
            setCursorPos();
            invalidate();
        }
    }

    void cursorRight() {
        if (_cursorX < (_cols-1)) {
            _cursorX++;
            setCursorPos();
            invalidate();
        }
    }

    void setCursorVisible(bool visible_){
        cursor.setVisible(visible_);
    }

    const uint8_t& cursorX() { return _cursorX; }
    const uint8_t& cursorY() { return _cursorY; }
    const int& cursorCX() { return cursor.CX(); }
    const int& cursorCY() { return cursor.CY(); }
    const int& colPitch() { return _colPitch; }
    const int& rowPitch() { return _rowPitch; }
    

private:
    CursorWidget cursor;

    uint8_t _rows = 2, _cols = 2;
    uint8_t _cursorY = 0, _cursorX = 0;

    int _colPitch;
    int _rowPitch;

    int hx, hy;
    
    void setCursorPos() {
        cursor.setPosition(_x + _cursorX*_colPitch+hx, _y + _cursorY*_rowPitch+hy);
    }
};
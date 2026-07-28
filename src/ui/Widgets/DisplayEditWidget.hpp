#pragma once
#include "UI/Widgets/Widget.hpp"
#include "SelectableText.hpp"
#include "Display/Display.hpp"
#include "DataModel/DataModel.hpp"
#include "UI/GFX.h"

class DisplayEditWidget : public Widget {
public:
    DisplayEditWidget(int x, int y, TelemetryType type=TelemetryType::Speed)
        : Widget(x, y),
        _type(type) {}

    void render() override {
        //Serial.printf("render idx? menu=%d inMode=%d x=%d y=%d\n", _menu, _inMode, _x, _y);
        if (!visible) {
            Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);
            return;
        }

        // Clear wherever we were last drawn, if it's different from where we are now
        if (_lastRenderX != _x || _lastRenderY != _y ||
            _lastRenderW != _width || _lastRenderH != _height) {
            if (_lastRenderW > 0 && _lastRenderH > 0) {
                Disp::fillRect(_lastRenderX, _lastRenderY, _lastRenderW, _lastRenderH, ST77XX_BLACK);
            }
        }

        uint16_t colour = ST77XX_BLUE;
        if ( _inMode || _menu) colour = ST77XX_GREEN;

        Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);  // clear own area first
        Disp::drawRect(_x, _y, _width, _height, colour);        // body border

        printTextCentered();

        if (_menu) drawMenuArrows();
        _menuLast = _menu;
        if (_inMode) drawModeArrows();
        _inModeLast = _inMode;
    }

    void setMode(WidgetSubMode m) {
        if (_mode != m) {
            _mode = m;
            switch (_mode) {
                case WidgetSubMode::CHANGE_TYPE: setText(toString(_type)); break;
                case WidgetSubMode::MOVE: setText("Move"); break;
                case WidgetSubMode::RESIZE: setText("Resize"); break;
                case WidgetSubMode::DONE: setText("Done"); break;
                case WidgetSubMode::DELETE: setText("Delete?"); break;
            }
        }
    };

    void setType(TelemetryType type) {
        if (type != _type) {
            _type = type;
            if (_mode == WidgetSubMode::CHANGE_TYPE) {
                setText(toString(_type));
            }
        }
    }

    const TelemetryType& type() const { return _type; }

    void drawUpArrow() {
        Disp::fillTriangle(_x + _width/2, _y, _x + _width/2 + 8, _y + 8, _x + _width/2 - 8, _y + 8, ST77XX_WHITE);
    }

    void drawDownArrow() {
        Disp::fillTriangle(_x + _width/2, _y + _height, _x + _width/2 + 8, _y + _height - 8, _x + _width/2 - 8, _y + _height - 8, ST77XX_WHITE);
    }
    
    void drawLeftArrow() {
        Disp::fillTriangle(_x, _y + _height/2, _x + 8, _y + _height/2 - 8, _x + 8, _y + _height/2 + 8, ST77XX_WHITE);
    }

    void drawRightArrow() {
        Disp::fillTriangle(_x + _width, _y + _height/2, _x + _width - 8, _y + _height/2 - 8, _x + _width - 8, _y + _height/2 + 8, ST77XX_WHITE);
    }

    void drawModeArrows() {
        switch (_mode) {
            case WidgetSubMode::CHANGE_TYPE:
                drawUpArrow();
                drawDownArrow();
                break;
            
            case WidgetSubMode::MOVE:
            case WidgetSubMode::RESIZE:
                drawUpArrow();
                drawDownArrow();
                drawLeftArrow();
                drawRightArrow();
                break;
        }
    }

    void drawMenuArrows() {
        drawLeftArrow();
        drawRightArrow();
    }

    void setMenu(bool menu) { 
        if (_menu != menu) invalidate();
        _menu = menu;
    }

    void setInMode(bool inMode) { 
        if (_inMode != inMode) invalidate();
        _inMode = inMode;
    }

    void setSize(int w, int h) {
        bool changed = (w != _width || h != _height);
        Widget::setSize(w,h);
        if (changed) { calcTextCoords(); invalidate(); }
    }

private:
    TelemetryType _type;
    WidgetSubMode _mode = WidgetSubMode::CHANGE_TYPE;
    bool _menu = false, _menuLast = false;;
    bool _inMode = false, _inModeLast = false;
    int _lastRenderX = -1, _lastRenderY = -1, _lastRenderW = -1, _lastRenderH = -1;

    static constexpr uint8_t MAX_TEXT_SIZE = 8;
    
    String _text;
    uint8_t _size = 1;
    int _offsetX = 0;
    int _offsetY = 0;

    void calcTextCoords() {
        int16_t x1, y1;
        uint16_t w =0,  h = 0;

        _size = 0;
        uint16_t prevW = 0, prevH = 0;
        while (_size < MAX_TEXT_SIZE) {
            Disp::setTextSize(_size + 1);
            Disp::getTextBounds(_text, 0, 0, &x1, &y1, &w, &h);
            if (w >= _width || h >= _height) break;   // this size doesn't fit — stop, use previous
            _size++;
            prevW = w; prevH = h;
        }
        w = prevW; h = prevH;

        int hx = w/2;
        int hy = h/2;

        _offsetX = _width/2 - hx;
        _offsetY = _height/2 - hy;
    }

    void printTextCentered() {
        Disp::setTextSize(_size > 0 ? _size : 1);
        Disp::setCursor(_x + _offsetX, _y + _offsetY);
        Disp::print(_text);
    }

    void setText(String text) {
        if (_text != text) {
            invalidate();
            _text = text;
            calcTextCoords();
            invalidate();
        }
    }
};
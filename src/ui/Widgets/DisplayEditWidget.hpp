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

    void update(float dt) override {
        //setText(toString(_type));
    }

    void render() override {
        if (!visible) {
            Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);
            return;
        }

        Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);      // clear own area first
        Disp::drawRect(_x, _y, _width, _height, ST77XX_WHITE);      // body border

        switch (_mode) {
            case WidgetSubMode::CHANGE_TYPE:
                printTextCentered(toString(_type));
                drawUpArrow();
                drawDownArrow();
                break;
            
            case WidgetSubMode::MOVE:
                printTextCentered("Move");
                drawArrows();
                break;

            case WidgetSubMode::RESIZE:
                printTextCentered("Resize");
                drawArrows();
                break;

            case WidgetSubMode::DONE:
                printTextCentered("Done");
                break;

            case WidgetSubMode::DELETE:
                printTextCentered("Delete?");
                break;
        }
    }

    void setMode(WidgetSubMode m) {
        if (_mode != m) invalidate();
        _mode = m;
    };

    void printTextCentered(String text) {
        int16_t x = _x;
        int16_t y = _y + _height/2;
        int16_t x1, y1;
        uint16_t w =0,  h = 0;

        uint8_t size = 0;
        uint16_t prevW = 0, prevH = 0;
        while (size < MAX_TEXT_SIZE) {
            Disp::setTextSize(size + 1);
            Disp::getTextBounds(text, x, y, &x1, &y1, &w, &h);
            if (w >= _width || h >= _height) break;   // this size doesn't fit — stop, use previous
            size++;
            prevW = w; prevH = h;
        }
        Disp::setTextSize(size > 0 ? size : 1);
        w = prevW; h = prevH;

        int hx = w/2;
        int hy = h/2;

        int cx = _x + _width/2 - hx;
        int cy = _y + _height/2 - hy;

        Disp::setCursor(cx , cy);
        Disp::print(text);
    }

    void setType(TelemetryType type) {
        if (type != _type) invalidate();
        _type = type;
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

    void drawArrows() {
        drawUpArrow();
        drawDownArrow();
        drawLeftArrow();
        drawRightArrow();
    }

private:
    TelemetryType _type;
    WidgetSubMode _mode = WidgetSubMode::CHANGE_TYPE;

    uint8_t MAX_TEXT_SIZE = 8;
};
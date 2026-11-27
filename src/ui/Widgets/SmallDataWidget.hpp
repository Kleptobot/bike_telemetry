#pragma once
#include "ui/Widgets/Widget.hpp"
#include "display/Display.hpp"
#include "ui/GFX.h"

class SmallDataWidget : public Widget {
    public:
        SmallDataWidget(int x, int y)
            : Widget(x, y) {}
        
        void render() override {
            if (!visible) return;
            
            Disp::setTextSize(2);
            if (_data < 10)
                Disp::print(" ");
            if (_data < 100)
                Disp::print(" ");
            Disp::print(" ");
            Disp::printFloat(_data, 0);
            Disp::setTextSize(1);
            Disp::setCursor(_x+49, _y);
            Disp::print(_units);
        }
        
        void setData(float data) { _data = data; }
        void setUnits(String units) { _units = units; }

    private:
        float _data;
        String _units;
};
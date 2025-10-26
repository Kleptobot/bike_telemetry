#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"

class SmallDataWidget : public Widget {
    public:
        SmallDataWidget(int x, int y)
            : Widget(x, y) {}
        
        void render() override {
            if (!visible) return;
            
            if (_data < 10)
                Disp::print(" ");
            if (_data < 100)
                Disp::print(" ");
            Disp::print(" ");
            Disp::printFloat(_data, 0);
            Disp::setTextSize(1);
            Disp::setCursor(x+49, y);
            Disp::print(_units);
        }
        
        void setData(float data) { _data = data; }
        void setUnits(String units) { _units = units; }

    private:
        float _data;
        String _units;
};
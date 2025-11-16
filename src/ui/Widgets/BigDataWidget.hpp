#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"

class BigDataWidget : public Widget {
    public:
        BigDataWidget(int x, int y)
            : Widget(x, y) {}
        

        void render() override {
            if (!visible) {
                Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
                return;
            }

            Disp::setTextColor(_color);
            Disp::setTextSize(_size);
            Disp::setCursor(x, y);
            if (_value < 100)
                Disp::print(' ');
            if (_value < 10)
                Disp::print(' ');
            Disp::printFloat(_value, 0);
            Disp::setTextSize(2);
            Disp::setCursor(x+18*_size, y);
            Disp::print(labelForType(_type));
        }

        void setType(TelemetryType type) {
            _type = type;
            invalidate();
        }
        
        void setSize(uint8_t size) { 
            _size = size;
            int16_t x1,y1;
            uint16_t w,h;
            _width = 18*size + 6;
            _height= 8*_size;
        }

        void setColor(uint16_t color) { _color = color; }

        void update(const Telemetry& t) {
            _value = GetTelemetryValue(t, _type);
            invalidate();
        }
        
        void setUnits(String units) { _units = units; }

    private:
        TelemetryType _type = TelemetryType::Speed;
        float _value;
        String _units;
        uint8_t _size = 4;
        uint16_t _color = ST77XX_WHITE;

        const char* labelForType(TelemetryType t) const {
            switch (t) {
                case TelemetryType::Speed: return "km/h";
                case TelemetryType::Cadence: return "rpm";
                case TelemetryType::HeartRate: return "bpm";
                case TelemetryType::Temperature: return " C ";  //° cannot be rendered
                case TelemetryType::Power: return " W ";
                case TelemetryType::Altitude: return " m ";
                default: return "-";
            }
        }
};
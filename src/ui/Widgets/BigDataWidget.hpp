#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"
#include "HAL/SensorData.hpp"

class BigDataWidget : public Widget {
    public:
        BigDataWidget(int x, int y, int size=4, TelemetryType type=TelemetryType::Speed)
            : Widget(x, y),
            _size(size),
            _type(type) {
            _width = 18*size + 6;
            _height= 8*_size;
        }
        

        void render() override {
            if (!visible) {
                Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
                return;
            }
            if (_type == TelemetryType::Undefined) {
                Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
                return;
            }else if(_type == TelemetryType::Location) {
                Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
                uint16_t coordColor = _locationValue.valid ? ST77XX_GREEN : ST77XX_RED;
                Disp::setTextColor(coordColor);
                Disp::setTextSize(_size/2);
                Disp::setCursor(x, y);
                Disp::print("Lat:");
                Disp::printDouble(_locationValue.latitude, 6);
                Disp::setCursor(x, y + 8*(_size/2));
                Disp::print("Lng:");
                Disp::printDouble(_locationValue.longitude, 6);
                return;
            }else{
                Disp::setTextColor(_color);
                Disp::setTextSize(_size);
                Disp::setCursor(x, y);

                // Split into integer and decimal parts
                int intPart = (int)_value;
                int decPart = (int)((_value - intPart) * 10); // one decimal digit

                // Print integer part
                if (intPart < 10)
                    Disp::print(' ');
                if (intPart < 100)
                    Disp::print(' ');
                Disp::print(intPart);

                // Switch to half size for decimal part, aligned to top
                int decX = Disp::getCursorX();
                int decY = y; // top-aligned, same Y as the integer part
                Disp::setTextSize(_size / 2 > 0 ? _size / 2 : 1);
                Disp::setCursor(decX, decY);
                Disp::print('.');
                Disp::print(decPart);

                // Place label after decimal, back to full size
                int labelX = Disp::getCursorX();
                Disp::setTextSize(2);
                Disp::setCursor(labelX, y);
                Disp::print(labelForType(_type));
            }
        }

        void setType(TelemetryType type) {
            _type = type;
            invalidate();
        }
        
        void setSize(uint8_t size) { 
            _size = size;
            _width = 18*size + 6;
            _height= 8*_size;
        }

        void setColor(uint16_t color) { _color = color; }

        void update(const Telemetry& t) {
            if (_type == TelemetryType::Undefined) return;
            auto newVal = GetTelemetryValue(t, _type);
            if (std::holds_alternative<float>(newVal)) {
                _value = std::get<float>(newVal);
                if (_value != _value) { // Check for NaN
                    _value = 0.0f;
                }
            } else {
                _locationValue = std::get<location_data>(newVal);
            }
            invalidate();
        }
        
        void setUnits(String units) { _units = units; }

    private:
        uint8_t _size;
        TelemetryType _type;
        float _value;
        location_data _locationValue;
        String _units;
        uint16_t _color = ST77XX_WHITE;

        const char* labelForType(TelemetryType t) const {
            switch (t) {
                case TelemetryType::Speed: return "km/h";
                case TelemetryType::Cadence: return "rpm";
                case TelemetryType::HeartRate: return "bpm";
                case TelemetryType::Temperature: return " C ";  //° cannot be rendered
                case TelemetryType::Power: return " W ";
                case TelemetryType::Altitude: return " m ";
                case TelemetryType::Distance: return " m ";
                case TelemetryType::TotalDist: return " km";
                case TelemetryType::Location: return "";
                default: return "-";
            }
        }
};
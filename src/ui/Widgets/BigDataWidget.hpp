#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"
#include "HAL/SensorData.hpp"
#include "DataModel/DataModel.hpp"

class BigDataWidget : public Widget {
    public:
        BigDataWidget(int x, int y, int size=4, TelemetryType type=TelemetryType::Speed)
            : Widget(x, y),
            _size(size),
            _type(type) {
            _width = 36*size + 6;
            _height= 8*_size;
            // ensure type-specific sizing (e.g., Location) is applied
            setSize(_size);
        }
        

        void render() override {
            if (!visible) {
                Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
                return;
            }
            if (_type == TelemetryType::Undefined) {
                Disp::fillRect(x,y,_width,_height,ST77XX_BLACK);
                return;
            }
            Disp::setTextColor(_color);
            Disp::setTextSize(_size);
            Disp::setCursor(x, y);
            if(_type == TelemetryType::Location) {
                if(!_locationValue.valid) {
                    Disp::print("No GPS");
                    return;
                }
                Disp::setTextSize(_size/2);
                Disp::print("Lat:");
                Disp::print(formatLocationValue(_locationValue.latitude));
                Disp::setCursor(x, y + 8*(_size/2));
                Disp::print("Lng:");
                Disp::print(formatLocationValue(_locationValue.longitude));
                return;
            }else{
                // Split into integer and decimal parts
                int intPart = (int)_value;
                int decPart = (int)(float(_value - intPart) * 10.0f); // one decimal digit

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
                String debugOutput = "For type: " + String(toString(_type)) +" Value: " + String(_value);
                if (ENABLE_INVALIDATE_DEBUG) {
                    Serial.println("[BigDataWidget] " + debugOutput);
                }
            }
        }

        void setType(TelemetryType type) {
            _type = type;
            setSize(_size); // recalculate width/height based on type
            invalidate();
        }
        
        void setSize(uint8_t size) { 
            _size = size;
            if (_type == TelemetryType::Location) {
                const uint8_t ts = max<uint8_t>(1, _size / 2);
                _width = (4 + 9) * 6 * ts + 8; // "Lat:" + value, plus padding
                _height = 16 * ts + 4;        // two lines of half-size text plus padding
            } else {
                _width = 36 * size + 6;
                _height = 8 * _size;
            }
        }

        void setColor(uint16_t color) { _color = color; }

        void update(const Telemetry& t) {
            if (_type == TelemetryType::Undefined) return;
            auto newVal = GetTelemetryValue(t, _type);
            bool newData = false;
            if (std::holds_alternative<float>(newVal)) {
                newData = _value != std::get<float>(newVal);
                _value = std::get<float>(newVal);
                if (_value != _value) { // Check for NaN
                    _value = 0.0f;
                }
                if (_type == TelemetryType::HeartRate && _value > 0) {
                    //get the zone thresholds from the model and set color accordingly
                    DataModel& model = App::instance().getModel();
                    int hr = static_cast<int>(_value);
                    auto appData = model.app().get();
                    if (hr < appData.zone1Start) _color = ST77XX_BLUE;
                    else if (hr < appData.zone2Start) _color = ST77XX_GREEN;
                    else if (hr < appData.zone3Start) _color = ST77XX_YELLOW;
                    else if (hr < appData.zone4Start) _color = ST77XX_ORANGE;
                    else _color = ST77XX_RED;
                }
            } else {
                newData = _locationValue != std::get<location_data>(newVal);
                _locationValue = std::get<location_data>(newVal);
                _color = _locationValue.valid ? ST77XX_GREEN : ST77XX_RED;
            }
            if (newData) {
                invalidate();
            }
        }
        
        void setUnits(String units) { _units = units; }

    private:
        String formatLocationValue(double value) const {
            int maxLen = 9;
            int integerDigits = 1;
            double absVal = value < 0 ? -value : value;
            if (absVal >= 100) integerDigits = 3;
            else if (absVal >= 10) integerDigits = 2;
            int signChars = value < 0 ? 1 : 0;
            int decimals = maxLen - integerDigits - signChars - 1; // 1 for dot
            if (decimals < 0) decimals = 0;
            String formatted = String(value, decimals);
            while (formatted.length() > maxLen && decimals > 0) {
                decimals--;
                formatted = String(value, decimals);
            }
            if (formatted.length() > maxLen) {
                formatted = formatted.substring(0, maxLen);
            }
            return formatted;
        }

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
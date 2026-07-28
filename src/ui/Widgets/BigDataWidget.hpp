#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"
#include "HAL/SensorData.hpp"
#include "DataModel/DataModel.hpp"

class BigDataWidget : public Widget {
public:
    BigDataWidget(int x, int y, int w, int h, TelemetryType type=TelemetryType::Speed)
        : Widget(x, y),
        _type(type) {
        _width = w;
        _height= h;
        unitStr = String(labelForType(_type));

        // Find the largest integer-part text size where everything still fits the footprint.
        // Decimal/unit text is always drawn at decimalSizeFor(intSize), roughly 1/3 the integer size.
        int16_t bx, by;
        bestSize = 1;

        for (uint8_t s = 1; s <= MAX_BIG_TEXT_SIZE; s++) {
            uint8_t decSize = decimalSizeFor(s);

            Disp::setTextSize(s);
            Disp::getTextBounds("888", 0, 0, &bx, &by, &intW, &intH);

            Disp::setTextSize(decSize);
            Disp::getTextBounds(".*", 0, 0, &bx, &by, &decW, &decH);
            Disp::getTextBounds(unitStr, 0, 0, &bx, &by, &unitW, &unitH);

            uint16_t totalW = intW + max(decW, unitW);
            uint16_t stackedH = decH + UNIT_GAP_PX + unitH;
            uint16_t totalH = max(intH, stackedH);

            if (totalW > _width || totalH > _height) break;   // this size no longer fits — stop, keep previous
            bestSize = s;
        }
        
        Disp::setTextSize(bestSize);
        Disp::getTextBounds("888", 0, 0, &bx, &by, &intW, &intH);
        uint8_t decSize = decimalSizeFor(bestSize);
        Disp::setTextSize(decSize);
        Disp::getTextBounds(".*", 0, 0, &bx, &by, &decW, &decH);
        Disp::getTextBounds(unitStr, 0, 0, &bx, &by, &unitW, &unitH);
    }
    

    void render() override {
        if (!visible) {
            Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);
            return;
        }
        if (_type == TelemetryType::Undefined) {
            Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);
            return;
        }

        Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);   // clear stale pixels from a differently-sized previous value
        Disp::setTextColor(_color);

        if (_type == TelemetryType::Location) {
            Disp::setTextSize(1);
            Disp::setCursor(_x, _y);
            if (!_locationValue.valid) {
                Disp::print("No GPS");
                return;
            }
            Disp::print("Lat:");
            Disp::print(formatLocationValue(_locationValue.latitude));
            Disp::setCursor(_x, _y + 8);
            Disp::print("Lng:");
            Disp::print(formatLocationValue(_locationValue.longitude));
            return;
        }

        // Build the three text blocks

        // Re-measure at the size that actually fits, so layout math below is correct
        uint8_t decSize = decimalSizeFor(bestSize);

        // Integer part: top-left anchored at widget origin
        Disp::setTextSize(bestSize);
        Disp::setCursor(_x, _y);
        Disp::print(intStr);

        // Decimal part: same top (_y) as the integer part, immediately to its right
        int decX = _x + intW;
        Disp::setTextSize(decSize);
        Disp::setCursor(decX, _y);
        Disp::print(decStr);

        // Units: directly below the decimal part, same x, small gap
        int unitY = _y + decH + UNIT_GAP_PX;
        Disp::setCursor(decX, unitY);
        Disp::print(unitStr);

        if (ENABLE_INVALIDATE_DEBUG) {
            String debugOutput = "For type: " + String(toString(_type)) + " Value: " + String(_value);
            Serial.println("[BigDataWidget] " + debugOutput);
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
                auto appData = model.bio().get();
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
            
            int intPart = (int)_value;
            int decPart = abs((int)((_value - intPart) * 10.0f));
            intStr = padIntPart(intPart);
            decStr = "." + String(decPart);
        }
    }
    
    void setUnits(String units) { _units = units; }

private:
    static constexpr uint8_t MAX_BIG_TEXT_SIZE = 10;
    static constexpr uint8_t UNIT_GAP_PX = 2;

    TelemetryType _type;
    float _value;
    location_data _locationValue;
    String _units;
    uint16_t _color = ST77XX_WHITE;
    
    uint16_t intW = 0, intH = 0, decW = 0, decH = 0, unitW = 0, unitH = 0;
    uint8_t bestSize;
    String unitStr, intStr, decStr;
    
    
    String formatLocationValue(double value) const {
        uint16_t maxLen = 9;
        uint16_t integerDigits = 1;
        double absVal = value < 0 ? -value : value;
        if (absVal >= 100) integerDigits = 3;
        else if (absVal >= 10) integerDigits = 2;
        uint16_t signChars = value < 0 ? 1 : 0;
        uint16_t decimals = maxLen - integerDigits - signChars - 1; // 1 for dot
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

    uint8_t decimalSizeFor(uint8_t intSize) const {
        uint8_t s = intSize / 3;
        return s > 0 ? s : 1;
    }

    String padIntPart(int intPart) const {
        String s = String(intPart);
        while (s.length() < 3) s = " " + s;
        return s;
    }

    const char* labelForType(TelemetryType t) const {
        switch (t) {
            case TelemetryType::Speed: return "km/h";
            case TelemetryType::Cadence: return "rpm";
            case TelemetryType::HeartRate: return "bpm";
            case TelemetryType::Temperature: return "C  ";  //° cannot be rendered
            case TelemetryType::Power: return "W  ";
            case TelemetryType::Altitude: return "m  ";
            case TelemetryType::Distance: return "m  ";
            case TelemetryType::TotalDist: return "km ";
            case TelemetryType::Location: return "";
            case TelemetryType::Grade: return "%  ";
            default: return " - ";
        }
    }
};
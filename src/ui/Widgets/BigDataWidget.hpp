#pragma once
#include "UI/Widgets/Widget.hpp"
#include "Display/Display.hpp"
#include "UI/GFX.h"
#include "HAL/SensorData.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/DataBus.hpp"

class BigDataWidget : public Widget {
public:
    BigDataWidget(int x, int y, int w, int h, TelemetryType type=TelemetryType::Speed)
        : Widget(x, y),
        _type(type),
        _busTopic(topicForType(type)),
        _subscribed(false) {
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
    

    // update(float dt) override for subscribed widgets.
    // When subscribed to DataBus, the callback sets _dirty when the value changes.
    // This method checks the flag and updates display strings + invalidates.
    // For non-subscribed widgets, this is a no-op (they use poll()).
    void update(float dt) override {
        if (!_subscribed || !_dirty) return;
        _dirty = false;

        invalidate();
        formatValue();
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

        if (_type == TelemetryType::Coasting ||
            _type == TelemetryType::HrZone || _type == TelemetryType::PowerZone) {
            // State rendered as text, not a number: centred, size 2.
            Disp::setTextSize(2);
            int16_t bx, by;
            uint16_t w, h;
            Disp::getTextBounds(_coastText, 0, 0, &bx, &by, &w, &h);
            Disp::setCursor(_x + (_width - (int16_t)w) / 2,
                            _y + (_height - (int16_t)h) / 2);
            Disp::print(_coastText);
            return;
        }

        // Build the three text blocks

        // Re-measure at the size that actually fits, so layout math below is correct
        uint8_t decSize = decimalSizeFor(bestSize);

        // Integer part: top-left anchored at widget origin
        int intY = _y + (_height - intH) / 2;
        Disp::setTextSize(bestSize);
        Disp::setCursor(_x, intY);
        Disp::print(intStr);

        // Decimal part: same top (_y) as the integer part, immediately to its right
        int decX = _x + intW;
        Disp::setTextSize(decSize);
        Disp::setCursor(decX, intY);
        Disp::print(decStr);

        // Units: directly below the decimal part, same x, small gap
        int unitY = intY + decH + UNIT_GAP_PX;
        Disp::setCursor(decX, unitY);
        Disp::print(unitStr);

        if (_drawRightEdge) {
            Disp::drawLine(_x + _width - 1, _y, _x + _width - 1, _y + _height - 1, EDGE_LINE_COLOR);
        }
        if (_drawBottomEdge) {
            Disp::drawLine(_x, _y + _height - 1, _x + _width - 1, _y + _height - 1, EDGE_LINE_COLOR);
        }

        if (ENABLE_INVALIDATE_DEBUG) {
            String debugOutput = "For type: " + String(toString(_type)) + " Value: " + String(_value);
            Serial.println("[BigDataWidget] " + debugOutput);
        }
    }

    void setColor(uint16_t color) { _color = color; }

    // Polling path for widget types without a float DataBus subscription:
    // Distance/TotalDist (floats, just not subscribed), Location (struct),
    // Coasting (bool rendered as text), GpsSats (uint8 storage).
    void poll(const DataBus& bus) {
        if (_type == TelemetryType::Undefined) return;

        if (_type == TelemetryType::Location) {
            const location_data& lv = bus.get<location_data>(Topic::Location);
            if (lv != _locationValue) {
                _locationValue = lv;
                _color = lv.valid ? ST77XX_GREEN : ST77XX_RED;
                invalidate();
            }
            return;
        }

        if (_type == TelemetryType::Coasting) {
            const bool coasting = bus.get<bool>(Topic::Coasting);
            const char* text = coasting ? "COAST" : "PEDAL";
            if (_coastText != text) {
                _coastText = text;
                _color = coasting ? ST77XX_ORANGE : ST77XX_GREEN;
                invalidate();
            }
            return;
        }

        if (_type == TelemetryType::HrZone || _type == TelemetryType::PowerZone) {
            const uint8_t zone = (_type == TelemetryType::HrZone)
                                     ? bus.get<uint8_t>(Topic::HrZone)
                                     : bus.get<uint8_t>(Topic::PowerZone);
            const char tag = (_type == TelemetryType::HrZone) ? 'Z' : 'P';
            const String text = (zone >= 1 && zone <= 5) ? String(tag) + String((int)zone)
                                                         : String("--");
            if (_coastText != text) {
                _coastText = text;
                _color = ST77XX_WHITE;
                invalidate();
            }
            return;
        }

        float newVal = 0.0f;
        switch (_type) {
            case TelemetryType::Distance:  newVal = bus.get<float>(Topic::DistanceDeltaM); break;
            case TelemetryType::TotalDist: newVal = bus.get<float>(Topic::TotalDistanceM) / 1000.0f; break; // m -> km
            case TelemetryType::GpsSats:   newVal = (float)bus.get<uint8_t>(Topic::GpsSats); break;
            default: return;  // subscribed types are handled by the callback
        }

        if (newVal != _value) {
            _value = newVal;
            if (_value != _value) { // NaN guard
                _value = 0.0f;
            }
            invalidate();
            formatValue();
        }
    }
    
    void setUnits(String units) { _units = units; }

    void setEdges(bool drawRight, bool drawBottom) {
        _drawRightEdge = drawRight;
        _drawBottomEdge = drawBottom;
    }

    // --- DataBus subscription support ---
    // Returns true if this widget is subscribed to a DataBus topic.
    bool isSubscribed() const { return _subscribed; }

    // Static trampoline: stable function pointer usable for both subscribe
    // and unsubscribe. The widget's this arrives as ctx.
    static void busCallback(const void* data, void* ctx) {
        BigDataWidget* self = reinterpret_cast<BigDataWidget*>(ctx);
        if (self && data) {
            const float newValue = *reinterpret_cast<const float*>(data);
            // Change-gated: only reformat/re-invalidate when the value really
            // changed. The callback fires at loop rate (App publishes every
            // tick), so unconditional dirtying here made every render flush
            // the whole screen over SPI and rebuilt Strings continuously.
            // _value starts as NaN (NaN != anything), so the FIRST publish
            // always marks dirty -- a value that arrives as 0 and stays 0
            // still formats and renders exactly once.
            if (self->_value != newValue) {
                self->_value = newValue;
                self->_dirty = true;
            }
        }
    }

    // Subscribe to the DataBus topic corresponding to this widget's TelemetryType.
    // The callback only sets a dirty flag - it does NOT call invalidate()
    // because the callback runs synchronously during DataBus::publish(),
    // which may be in a context where display operations are unsafe.
    void subscribe(DataBus& bus) {
        if (_busTopic == Topic::TopicCount) return;  // No topic mapping
        if (_subscribed) return;  // Already subscribed

        _subscribed = bus.subscribe(_busTopic, &BigDataWidget::busCallback, this);
        if (_subscribed) _bus = &bus;
    }

    // Detach from the bus. Destroying a subscribed widget without this would
    // leave a dangling ctx pointer in the slot: the next publish() would
    // dereference freed memory. Critical because widgets live in a
    // std::vector that is clear()ed and rebuilt on every screen entry.
    ~BigDataWidget() override {
        if (_subscribed && _bus) {
            _bus->unsubscribe(_busTopic, &BigDataWidget::busCallback, this);
            _subscribed = false;
            _bus = nullptr;
        }
    }

private:
    static constexpr uint8_t MAX_BIG_TEXT_SIZE = 10;
    static constexpr uint8_t UNIT_GAP_PX = 2;

    TelemetryType _type;
    Topic _busTopic;       // Mapped DataBus topic for this TelemetryType
    bool _subscribed = false;
    bool _dirty = false;   // Set by callback when value changes; cleared by update()
    DataBus* _bus = nullptr;  // Bus this widget is subscribed to (for unsubscribe)
    float _value = NAN;    // NaN sentinel: first publish always differs (see busCallback)
    location_data _locationValue;
    String _coastText = "";   // Coasting tile: "PEDAL" / "COAST"
    String _units;
    uint16_t _color = ST77XX_WHITE;
    
    uint16_t intW = 0, intH = 0, decW = 0, decH = 0, unitW = 0, unitH = 0;
    uint8_t bestSize;
    String unitStr, intStr, decStr;

    bool _drawRightEdge = false;
    bool _drawBottomEdge = false;
    static constexpr uint16_t EDGE_LINE_COLOR = 0x4208;   // dim grey
    
    String formatLocationValue(double value) const {
        uint16_t maxLen = 9;
        uint16_t integerDigits = 1;
        double absVal = value < 0 ? -value : value;
        if (absVal >= 100) integerDigits = 3;
        else if (absVal >= 10) integerDigits = 2;
        uint16_t signChars = value < 0 ? 1 : 0;
        int16_t decimals = maxLen - integerDigits - signChars - 1; // 1 for dot
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

    // Units label comes from the shared type table in TelemetryType.hpp --
    // one source of truth for the vocabulary.
    const char* labelForType(TelemetryType t) const {
        return unitsForType(t);
    }

    // Decimal places to display for each type. Types not listed default to 1.
    uint8_t decimalsForType(TelemetryType t) const {
        switch (t) {
            case TelemetryType::GearRatio:
            case TelemetryType::Accel:
            case TelemetryType::GpsHdop:
            case TelemetryType::Torque:
            case TelemetryType::BatteryVolts:
            case TelemetryType::IntensityFactor:
                return 2;
            case TelemetryType::Ascent:
            case TelemetryType::Descent:
            case TelemetryType::GpsCourse:
            case TelemetryType::Calories:
            case TelemetryType::NormalizedPower:
            case TelemetryType::Tss:
                return 0;
            default:
                return 1;
        }
    }

    // Rebuild intStr/decStr from _value honouring decimalsForType().
    // Shared by the subscription path (update(float)) and the polling path
    // (poll()), which used to duplicate this block.
    void formatValue() {
        int intPart = (int)_value;
        const uint8_t decimals = decimalsForType(_type);
        intStr = padIntPart(intPart);
        if (decimals == 0) {
            decStr = "";
            return;
        }
        int scale = 1;
        for (uint8_t i = 1; i < decimals; i++) scale *= 10;
        int decPart = abs((int)((_value - intPart) * 10.0f * scale));
        decStr = "." + String(decPart);
    }
    // Map TelemetryType to a DataBus Topic for SUBSCRIPTION (float-backed
    // storage only -- the callback casts the slot to float). Types with
    // non-float storage or special rendering return TopicCount and go
    // through poll() instead: Location (struct), Coasting (bool),
    // GpsSats (uint8).
    static Topic topicForType(TelemetryType t) {
        switch (t) {
            // Derived
            case TelemetryType::Speed:       return Topic::SelectedSpeed;
            case TelemetryType::Altitude:    return Topic::FusedAltitude;
            case TelemetryType::BaroAlt:     return Topic::BaroAlt;
            case TelemetryType::Grade:       return Topic::SmoothedGrade;
            case TelemetryType::Vario:       return Topic::Vario;
            case TelemetryType::GearRatio:   return Topic::GearRatio;
            case TelemetryType::Energy:      return Topic::Energy;
            case TelemetryType::Accel:       return Topic::Acceleration;
            case TelemetryType::EstPower:    return Topic::EstimatedPower;
            case TelemetryType::Ascent:      return Topic::TotalAscent;
            case TelemetryType::Descent:     return Topic::TotalDescent;
            case TelemetryType::Calories:    return Topic::Calories;
            case TelemetryType::NormalizedPower: return Topic::NormalizedPower;
            case TelemetryType::IntensityFactor: return Topic::IntensityFactor;
            case TelemetryType::Tss:         return Topic::Tss;
            // Measured
            case TelemetryType::Cadence:     return Topic::Cadence;
            case TelemetryType::HeartRate:   return Topic::HeartRate;
            case TelemetryType::Power:       return Topic::PowerMeter;
            case TelemetryType::Temperature: return Topic::Temperature;
            case TelemetryType::GpsSpeed:    return Topic::GpsSpeed;
            case TelemetryType::GpsAlt:      return Topic::GpsAltitude;
            case TelemetryType::GpsCourse:   return Topic::GpsCourse;
            case TelemetryType::GpsHdop:     return Topic::GpsHdop;
            case TelemetryType::PedalBalance: return Topic::PedalBalance;
            case TelemetryType::Torque:      return Topic::TorqueNm;
            case TelemetryType::BatteryVolts: return Topic::BatteryVolts;
            // Distance/TotalDist are floats but poll (cheap, low rate)
            default: return Topic::TopicCount;
        }
    }
};
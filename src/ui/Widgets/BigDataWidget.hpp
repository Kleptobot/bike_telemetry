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
    // For non-subscribed widgets, this is a no-op (they use update(const Telemetry&)).
    void update(float dt) override {
        if (!_subscribed || !_dirty) return;
        _dirty = false;

        invalidate();

        int intPart = (int)_value;
        int decPart = abs((int)((_value - intPart) * 10.0f));
        intStr = padIntPart(intPart);
        decStr = "." + String(decPart);
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

    // Polling path for widget types without a DataBus subscription
    // (Distance, TotalDist, Location). Reads the bus directly; the old
    // Telemetry-struct variant and GetTelemetryValue are gone.
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

        float newVal = 0.0f;
        switch (_type) {
            case TelemetryType::Distance:  newVal = bus.get<float>(Topic::DistanceDeltaM); break;
            case TelemetryType::TotalDist: newVal = bus.get<float>(Topic::TotalDistanceM) / 1000.0f; break; // m -> km
            default: return;  // subscribed types are handled by the callback
        }

        if (newVal != _value) {
            _value = newVal;
            if (_value != _value) { // NaN guard
                _value = 0.0f;
            }
            invalidate();

            int intPart = (int)_value;
            int decPart = abs((int)((_value - intPart) * 10.0f));
            intStr = padIntPart(intPart);
            decStr = "." + String(decPart);
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
            // Always update value and set dirty flag, even if the value
            // hasn't changed. Without this, a widget displaying 0 (e.g.,
            // stationary speed) would never format its display strings
            // because _value starts at 0 and the first publish is also 0.
            self->_value = *reinterpret_cast<const float*>(data);
            self->_dirty = true;  // Signal that new data arrived
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
    float _value;
    location_data _locationValue;
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

    // Map TelemetryType to DataBus Topic for subscription.
    // Returns Topic::TopicCount for types that have no DataBus mapping.
    static Topic topicForType(TelemetryType t) {
        switch (t) {
            case TelemetryType::Speed:       return Topic::SelectedSpeed;
            case TelemetryType::Cadence:     return Topic::Cadence;
            case TelemetryType::Altitude:    return Topic::FusedAltitude;
            case TelemetryType::Grade:       return Topic::SmoothedGrade;
            case TelemetryType::HeartRate:   return Topic::HeartRate;
            case TelemetryType::Power:       return Topic::PowerMeter;
            case TelemetryType::Temperature: return Topic::Temperature;
            // Distance, TotalDist, Location: no direct topic mapping
            default: return Topic::TopicCount;
        }
    }
};
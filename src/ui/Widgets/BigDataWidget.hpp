#pragma once
#include "ui/Widgets/Widget.hpp"
#include "display/Display.hpp"
#include "ui/GFX.h"
#include "HAL/SensorData.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/DataBus.hpp"

class BigDataWidget : public Widget {
public:
    BigDataWidget(int x, int y, int w, int h, Topic topic=Topic::SelectedSpeed)
        : Widget(x, y),
        _topic(topic),
        _subscribed(false) {
        _width = w;
        _height= h;
        const TopicMetadata* info = topicMetadata(_topic);
        unitStr = info ? String(info->units) : "";

        // Find a starting text size; formatValue() refits it to the actual value.
        // Decimal/unit text is always drawn at decimalSizeFor(intSize), roughly 1/3 the integer size.
        // Time tiles render mm:ss, so measure with a 5-char sample, not "888".
        const char* intSample = isTimeType() ? "88:88" : "888";
        fitTextLayout(intSample, ".*");
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
        const TopicMetadata* info = topicMetadata(_topic);
        if (!info) {
            Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);
            return;
        }

        Disp::fillRect(_x, _y, _width, _height, ST77XX_BLACK);   // clear stale pixels from a differently-sized previous value
        Disp::setTextColor(_color);

        if (info->displayFormat == TopicDisplayFormat::Location) {
            if (_width < LOCATION_MIN_WIDTH_PX || _height < LOCATION_MIN_HEIGHT_PX) return;
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

        if (isStatusType()) {
            // State rendered as text, not a number: centred, size 2.
            Disp::setTextSize(2);
            int16_t bx, by;
            uint16_t w, h;
            Disp::getTextBounds(_statusText, 0, 0, &bx, &by, &w, &h);
            Disp::setCursor(_x + (_width - (int16_t)w) / 2,
                            _y + (_height - (int16_t)h) / 2);
            Disp::print(_statusText);
            return;
        }

        // Build the three text blocks

        // Re-measure at the size that actually fits, so layout math below is correct
        uint8_t decSize = decimalSizeFor(bestSize);

        // Integer part: top-left anchored at widget origin
        const bool hasDecimal = decStr.length() != 0;
        const uint16_t stackedContentH = hasDecimal ? decH : intH;
        const uint16_t stackedH = unitStr.length() == 0
                                      ? 0
                                      : stackedContentH + UNIT_GAP_PX + unitH;
        const uint16_t totalH = max(intH, stackedH);
        const int verticalOffset = (_height > static_cast<int>(totalH))
                                       ? (_height - static_cast<int>(totalH)) / 2
                                       : 0;
        int intY = _y + verticalOffset;
        Disp::setTextSize(bestSize);
        Disp::setCursor(_x, intY);
        Disp::print(intStr);

        // Decimal part: same top (_y) as the integer part, immediately to its right
        int decX = _x + intW;
        Disp::setTextSize(decSize);
        Disp::setCursor(decX, intY);
        Disp::print(decStr);

        // Units sit under the decimal part, or centered under whole numbers.
        int unitX = decStr.length() == 0
                        ? _x + (max(intW, unitW) - unitW) / 2
                        : decX;
        int unitY = intY + (hasDecimal ? decH : intH) + UNIT_GAP_PX;
        Disp::setCursor(unitX, unitY);
        Disp::print(unitStr);

        if (_drawRightEdge) {
            // 3px thick edge, kept inside the widget bounds
            for (int i = 1; i <= 3; i++) {
                Disp::drawLine(_x + _width - i, _y, _x + _width - i, _y + _height - 1, EDGE_LINE_COLOR);
            }
        }
        if (_drawBottomEdge) {
            for (int i = 1; i <= 3; i++) {
                Disp::drawLine(_x, _y + _height - i, _x + _width - 1, _y + _height - i, EDGE_LINE_COLOR);
            }
        }

        if (ENABLE_INVALIDATE_DEBUG) {
            String debugOutput = "For type: " + String(topicDisplayName(_topic)) + " Value: " + String(_value);
            Serial.println("[BigDataWidget] " + debugOutput);
        }
    }

    void setColor(uint16_t color) { _color = color; }

    // Float topics are subscribed; other storage types and formatted statuses
    // are read here using the same metadata as CSV serialization.
    void poll(const DataBus& bus) {
        const TopicMetadata* info = topicMetadata(_topic);
        if (!info) return;

        if (info->displayFormat == TopicDisplayFormat::Location) {
            const location_data& lv = bus.get<location_data>(_topic);
            if (lv != _locationValue) {
                _locationValue = lv;
                _color = lv.valid ? ST77XX_GREEN : ST77XX_RED;
                invalidate();
            }
            return;
        }

        if (info->displayFormat == TopicDisplayFormat::BooleanStatus ||
            info->displayFormat == TopicDisplayFormat::CoastingStatus) {
            const bool value = bus.get<bool>(_topic);
            setStatusText(value ? info->trueText : info->falseText,
                          colorFor(value ? info->trueColor : info->falseColor));
            return;
        }

        if (info->displayFormat == TopicDisplayFormat::HrZone ||
            info->displayFormat == TopicDisplayFormat::PowerZone) {
            const uint8_t zone = bus.get<uint8_t>(_topic);
            const String text = (zone >= 1 && zone <= 5)
                                    ? String(info->zonePrefix) + String(static_cast<int>(zone))
                                    : String("--");
            setStatusText(text.c_str(), ST77XX_WHITE);
            return;
        }

        if (info->displayFormat == TopicDisplayFormat::GpsQuality) {
            const uint8_t quality = bus.get<uint8_t>(_topic);
            const char* text = info->qualityPoorText;
            TopicColor color = info->qualityPoorColor;
            if (quality == 0) {
                text = info->qualityGoodText;
                color = info->qualityGoodColor;
            } else if (quality == 1) {
                text = info->qualityFairText;
                color = info->qualityFairColor;
            }
            setStatusText(text, colorFor(color));
            return;
        }

        float newVal = 0.0f;
        switch (info->valueKind) {
            case TopicValueKind::Float:
                newVal = bus.get<float>(_topic);
                break;
            case TopicValueKind::UInt8:
                newVal = static_cast<float>(bus.get<uint8_t>(_topic));
                break;
            case TopicValueKind::UInt32:
                newVal = static_cast<float>(bus.get<uint32_t>(_topic));
                break;
            case TopicValueKind::Int16:
                newVal = static_cast<float>(bus.get<int16_t>(_topic));
                break;
            case TopicValueKind::Bool:
            case TopicValueKind::Location:
                return;
        }
        newVal *= info->displayScale;

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
            const TopicMetadata* info = topicMetadata(self->_topic);
            if (!info) return;
            const float newValue = *reinterpret_cast<const float*>(data) * info->displayScale;
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

    // Subscribe to float-valued topics; metadata defines their bus type.
    // The callback only sets a dirty flag - it does NOT call invalidate()
    // because the callback runs synchronously during DataBus::publish(),
    // which may be in a context where display operations are unsafe.
    void subscribe(DataBus& bus) {
        const TopicMetadata* info = topicMetadata(_topic);
        if (!info || info->valueKind != TopicValueKind::Float) return;
        if (_subscribed) return;  // Already subscribed

        _subscribed = bus.subscribe(_topic, &BigDataWidget::busCallback, this);
        if (_subscribed) _bus = &bus;
    }

    // Detach from the bus. Destroying a subscribed widget without this would
    // leave a dangling ctx pointer in the slot: the next publish() would
    // dereference freed memory. Critical because widgets live in a
    // std::vector that is clear()ed and rebuilt on every screen entry.
    ~BigDataWidget() override {
        if (_subscribed && _bus) {
            _bus->unsubscribe(_topic, &BigDataWidget::busCallback, this);
            _subscribed = false;
            _bus = nullptr;
        }
    }

private:
    static constexpr uint8_t MAX_BIG_TEXT_SIZE = 10;
    static constexpr uint8_t UNIT_GAP_PX = 2;
    static constexpr uint16_t LOCATION_MIN_WIDTH_PX = 13 * 6;
    static constexpr uint16_t LOCATION_MIN_HEIGHT_PX = 2 * 8;

    Topic _topic;
    bool _subscribed = false;
    bool _dirty = false;   // Set by callback when value changes; cleared by update()
    DataBus* _bus = nullptr;  // Bus this widget is subscribed to (for unsubscribe)
    float _value = NAN;    // NaN sentinel: first publish always differs (see busCallback)
    location_data _locationValue;
    String _statusText = "";
    String _units;
    uint16_t _color = ST77XX_WHITE;
    
    uint16_t intW = 0, intH = 0, decW = 0, decH = 0, unitW = 0, unitH = 0;
    uint8_t bestSize;
    String unitStr, intStr, decStr;

    bool _drawRightEdge = false;
    bool _drawBottomEdge = false;
    static constexpr uint16_t EDGE_LINE_COLOR = 0x4208;   // dim grey

    bool isStatusType() const {
        const TopicMetadata* info = topicMetadata(_topic);
        if (!info) return false;
        return info->displayFormat == TopicDisplayFormat::BooleanStatus ||
               info->displayFormat == TopicDisplayFormat::CoastingStatus ||
               info->displayFormat == TopicDisplayFormat::HrZone ||
               info->displayFormat == TopicDisplayFormat::PowerZone ||
               info->displayFormat == TopicDisplayFormat::GpsQuality;
    }

    static uint16_t colorFor(TopicColor color) {
        switch (color) {
            case TopicColor::White: return ST77XX_WHITE;
            case TopicColor::Green: return ST77XX_GREEN;
            case TopicColor::Orange: return ST77XX_ORANGE;
            case TopicColor::Red: return ST77XX_RED;
        }
        return ST77XX_WHITE;
    }

    void setStatusText(const char* text, uint16_t color) {
        if (_statusText != text || _color != color) {
            _statusText = text;
            _color = color;
            invalidate();
        }
    }
    
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

    void fitTextLayout(const String& integerText, const String& decimalText) {
        int16_t bx, by;
        bestSize = 1;
        for (uint8_t s = 1; s <= MAX_BIG_TEXT_SIZE; s++) {
            const uint8_t decSize = decimalSizeFor(s);
            uint16_t candidateIntW, candidateIntH;
            uint16_t candidateDecW, candidateDecH;
            uint16_t candidateUnitW, candidateUnitH;

            Disp::setTextSize(s);
            Disp::getTextBounds(integerText, 0, 0, &bx, &by, &candidateIntW, &candidateIntH);
            Disp::setTextSize(decSize);
            Disp::getTextBounds(decimalText, 0, 0, &bx, &by, &candidateDecW, &candidateDecH);
            Disp::getTextBounds(unitStr, 0, 0, &bx, &by, &candidateUnitW, &candidateUnitH);

            const uint16_t totalW = decimalText.length() == 0
                                        ? max(candidateIntW, candidateUnitW)
                                        : candidateIntW + max(candidateDecW, candidateUnitW);
            const bool hasDecimal = decimalText.length() != 0;
            const uint16_t stackedContentH = hasDecimal ? candidateDecH : candidateIntH;
            const uint16_t stackedH = unitStr.length() == 0
                                          ? 0
                                          : stackedContentH + UNIT_GAP_PX + candidateUnitH;
            const uint16_t totalH = max(candidateIntH, stackedH);
            const bool fits = totalW <= _width && totalH <= _height;

            if (s == 1 || fits) {
                bestSize = s;
                intW = candidateIntW;
                intH = candidateIntH;
                decW = candidateDecW;
                decH = candidateDecH;
                unitW = candidateUnitW;
                unitH = candidateUnitH;
            }
            if (!fits) break;
        }
    }

    String padIntPart(int intPart) const {
        String s = String(intPart);
        while (s.length() < 3) s = " " + s;
        return s;
    }

    uint8_t decimalsForType() const {
        const TopicMetadata* info = topicMetadata(_topic);
        return info ? info->decimals : 1;
    }

    // Rebuild intStr/decStr from _value honouring the topic metadata.
    // Shared by the subscription path (update(float)) and the polling path
    // (poll()), which used to duplicate this block.
    void formatValue() {
        // Time types render as mm:ss (up to hours via minutes: 75:30).
        if (isTimeType()) {
            uint32_t total = (uint32_t)max(0.0f, _value);
            uint32_t mm = total / 60, ss = total % 60;
            intStr = (mm < 10 ? "0" : "") + String(mm) + ":" + (ss < 10 ? "0" : "") + String(ss);
            decStr = "";
            fitTextLayout(intStr, decStr);
            return;
        }
        int intPart = (int)_value;
        const uint8_t decimals = decimalsForType();
        intStr = padIntPart(intPart);
        if (decimals == 0) {
            decStr = "";
            fitTextLayout(intStr, decStr);
            return;
        }
        int scale = 1;
        for (uint8_t i = 1; i < decimals; i++) scale *= 10;
        int decPart = abs((int)((_value - intPart) * 10.0f * scale));
        decStr = "." + String(decPart);
        fitTextLayout(intStr, decStr);
    }
    bool isTimeType() const {
        const TopicMetadata* info = topicMetadata(_topic);
        return info && (info->displayFormat == TopicDisplayFormat::ElapsedTime ||
                        info->displayFormat == TopicDisplayFormat::MovingTime);
    }

};
#ifndef LAYOUTDATAPROVIDER_H
#define LAYOUTDATAPROVIDER_H

#include <Arduino.h>
#include "TelemetryDataProvider.hpp"

enum class Edge {
    None,
    Left,
    Right,
    Top,
    Bottom
};

enum class WidgetSubMode { CHANGE_TYPE, MOVE, RESIZE, DELETE, DONE };

inline WidgetSubMode& operator++(WidgetSubMode& w) {
    switch(w){
        case WidgetSubMode::CHANGE_TYPE : w = WidgetSubMode::MOVE; break;
        case WidgetSubMode::MOVE : w = WidgetSubMode::RESIZE; break;
        case WidgetSubMode::RESIZE : w = WidgetSubMode::DELETE; break;
        case WidgetSubMode::DELETE : w = WidgetSubMode::DONE; break;
        case WidgetSubMode::DONE : w = WidgetSubMode::CHANGE_TYPE; break;
        default: w = WidgetSubMode::CHANGE_TYPE;
    }
    return w;
};

inline WidgetSubMode& operator--(WidgetSubMode& w) {
    switch(w){
        case WidgetSubMode::CHANGE_TYPE : w = WidgetSubMode::DONE; break;
        case WidgetSubMode::MOVE : w = WidgetSubMode::CHANGE_TYPE; break;
        case WidgetSubMode::RESIZE : w = WidgetSubMode::MOVE; break;
        case WidgetSubMode::DELETE : w = WidgetSubMode::RESIZE; break;
        case WidgetSubMode::DONE : w = WidgetSubMode::DELETE; break;
        default: w = WidgetSubMode::CHANGE_TYPE;
    }
    return w;
};

struct DisplayItem {
    TelemetryType type;
    uint8_t x0; //left
    uint8_t y0; //top
    uint8_t x1; //right
    uint8_t y1; //bottom

    Edge isNeighbour(const DisplayItem& other) const {
        // 1. Self-comparison check using memory addresses
        if (&other == this) return Edge::None;

        // 2. Is 'other' touching our RIGHT edge?
        if (other.x0 == x1 && (other.y0 < y1 && other.y1 > y0)) {
            return Edge::Right;
        }

        // 3. Is 'other' touching our LEFT edge?
        if (other.x1 == x0 && (other.y0 < y1 && other.y1 > y0)) {
            return Edge::Left;
        }

        // 4. Is 'other' touching our TOP edge?
        if (other.y0 == y1 && (other.x0 < x1 && other.x1 > x0)) {
            return Edge::Bottom;
        }

        // 5. Is 'other' touching our BOTTOM edge?
        if (other.y1 == y0 && (other.x0 < x1 && other.x1 > x0)) {
            return Edge::Top;
        }

        return Edge::None;
    }

    bool intersects(const DisplayItem& other) const {
        return (x0<other.x1) && (x1>other.x0) && (y0<other.y1) && (y1>other.y0);
    }

    bool isValidGeometry() const {
        return x0 < x1 &&
            y0 < y1;
    }

    bool insideGrid(uint8_t cols, uint8_t rows) const {
        return x0 < cols &&
            y0 < rows &&
            x1 <= cols &&
            y1 <= rows;
    }
};

struct LayoutData {
    std::vector<DisplayItem> displays;
    uint8_t rows;
    uint8_t cols;
};

class LayoutDataProvider {
public:
    const LayoutData& get() const { return _data; }
    uint32_t version() const { return _version; }

    void update(const LayoutData& newData) {
        _data = newData;
        ++_version;
    }

private:
    LayoutData _data{};
    uint32_t _version = 0;
};

#endif
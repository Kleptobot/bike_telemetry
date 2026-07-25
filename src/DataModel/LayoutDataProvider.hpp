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
            return Edge::Top;
        }

        // 5. Is 'other' touching our BOTTOM edge?
        if (other.y1 == y0 && (other.x0 < x1 && other.x1 > x0)) {
            return Edge::Bottom;
        }

        return Edge::None;
    }

    bool topLeft(uint8_t x, uint8_t y) {
        return (x0 == x) && (y0 == y);
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
#pragma once
#include <Arduino.h>
#include "TelemetryDataProvider.hpp"

struct LayoutData {
    std::vector<TelemetryType> displays;
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
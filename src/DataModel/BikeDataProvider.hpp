#ifndef BIKEDATAPROVIDER_H
#define BIKEDATAPROVIDER_H

#include <Arduino.h>
#include "TelemetryDataProvider.hpp"

struct BikeData {
    uint16_t mass = 10;                    // kg
    // Millimetres. Defaulted to a 700x25c wheel rather than left at zero:
    // BikeData is value-initialised, and a zero circumference makes
    // App::updateTelemetry compute a speed of exactly 0 from a live wheel
    // sensor, with no indication why.
    uint16_t wheelCircumference = 2105;
};

class BikeDataProvider {
public:
    const BikeData& get() const { return _data; }
    uint32_t version() const { return _version; }

    void update(const BikeData& newData) {
        _data = newData;
        ++_version;
    }

private:
    BikeData _data{};
    uint32_t _version = 0;
};

#endif
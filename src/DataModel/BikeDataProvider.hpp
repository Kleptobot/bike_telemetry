#ifndef BIKEDATAPROVIDER_H
#define BIKEDATAPROVIDER_H

#include <Arduino.h>
#include "TelemetryDataProvider.hpp"

struct BikeData {
    uint16_t mass;
    uint16_t wheelCircumference;
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
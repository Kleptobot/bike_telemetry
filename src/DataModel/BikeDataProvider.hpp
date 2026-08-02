#ifndef BIKEDATAPROVIDER_H
#define BIKEDATAPROVIDER_H

#include <Arduino.h>
#include "TelemetryDataProvider.hpp"


enum class LoggerType { FIT, TCX, CSV };

inline String loggerToString(LoggerType type) {
    switch (type) {
        case LoggerType::FIT: return "FIT";
        case LoggerType::TCX: return "TCX";
        case LoggerType::CSV: return "CSV";
        default: return "Unknown";
    }
}

inline LoggerType loggerFromString(const String& str) {
    if (str == "FIT") return LoggerType::FIT;
    if (str == "TCX") return LoggerType::TCX;
    if (str == "CSV") return LoggerType::CSV;
    return LoggerType::FIT; // Default to FIT if unknown
}

struct BikeData {
    uint16_t mass = 10;                    // kg
    // Millimetres. Defaulted to a 700x25c wheel rather than left at zero:
    // BikeData is value-initialised, and a zero circumference makes
    // App::updateTelemetry compute a speed of exactly 0 from a live wheel
    // sensor, with no indication why.
    uint16_t wheelCircumference = 2105;
    LoggerType logger = LoggerType::FIT;
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
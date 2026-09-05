#ifndef BIKEDATAPROVIDER_H
#define BIKEDATAPROVIDER_H

#include <Arduino.h>


enum class LoggerType { FIT, TCX, CSV };

// Auto-lap trigger source. One configured value serves both: kilometres when
// Distance, minutes when Time.
enum class AutoLapMode : uint8_t { Off = 0, Distance = 1, Time = 2 };

inline String autoLapModeToString(AutoLapMode mode) {
    switch (mode) {
        case AutoLapMode::Distance: return "Dist";
        case AutoLapMode::Time:     return "Time";
        default:                    return "Off";
    }
}

inline AutoLapMode autoLapModeFromString(const String& str) {
    if (str == "Dist") return AutoLapMode::Distance;
    if (str == "Time") return AutoLapMode::Time;
    return AutoLapMode::Off;
}

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
    // Auto-pause logging when the rider stops (speed-based with hysteresis).
    bool autoPause = true;
    // Minutes of no user input before the device sleeps while idle.
    // A stored 0 is clamped to the default at the point of use.
    uint8_t idleSleepMinutes = 5;
    // Auto-lap: fire a lap every autoLapValue kilometres (Distance) or
    // minutes (Time) of moving time. Off disables the trigger.
    AutoLapMode autoLapMode = AutoLapMode::Off;
    uint8_t autoLapValue = 10;
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
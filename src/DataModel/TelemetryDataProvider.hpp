#pragma once
#include <Arduino.h>

#include "HAL/SensorData.hpp"

enum class TelemetryType : uint8_t {
    Speed,
    Cadence,
    Temperature,
    Altitude,
    HeartRate,
    Power,
    Distance,
    Undefined
};

struct Telemetry {
    imu_data imu;
    dps_data dps;
    float speed;
    float cadence;
    float temperature;
    float altitude;
    float heartrate;
    float power;
    bool validLocation;
    double longitude;
    double latitude;
    float distance;
};

inline TelemetryType& operator++(TelemetryType& t) {
    switch(t){
        case TelemetryType::Speed : t = TelemetryType::Cadence; break;
        case TelemetryType::Cadence : t = TelemetryType::Temperature; break;
        case TelemetryType::Temperature : t = TelemetryType::Altitude; break;
        case TelemetryType::Altitude : t = TelemetryType::HeartRate; break;
        case TelemetryType::HeartRate : t = TelemetryType::Power; break;
        case TelemetryType::Power : t = TelemetryType::Distance; break;
        case TelemetryType::Distance : t = TelemetryType::Speed; break;
        default: t = TelemetryType::Undefined;
    }
    return t;
};

inline TelemetryType& operator--(TelemetryType& t) {
    switch(t){
        case TelemetryType::Speed : t = TelemetryType::Distance; break;
        case TelemetryType::Cadence : t = TelemetryType::Speed; break;
        case TelemetryType::Temperature : t = TelemetryType::Cadence; break;
        case TelemetryType::Altitude : t = TelemetryType::Temperature; break;
        case TelemetryType::HeartRate : t = TelemetryType::Altitude; break;
        case TelemetryType::Power : t = TelemetryType::HeartRate; break;
        case TelemetryType::Distance : t = TelemetryType::Power; break;
        default: t = TelemetryType::Undefined; break;
    }
    return t;
};

inline const char* toString(const TelemetryType& t) {
    switch(t){
        case TelemetryType::Speed : return "Speed"; break;
        case TelemetryType::Cadence : return"Cadence"; break;
        case TelemetryType::Temperature : return"Temperature"; break;
        case TelemetryType::Altitude : return"Altitude"; break;
        case TelemetryType::HeartRate : return"HeartRate"; break;
        case TelemetryType::Power : return"Power"; break;
        case TelemetryType::Distance : return"Distance"; break;
        default: return "-"; break;
    }
}

inline TelemetryType TelemetryTypefromString(String s) {
    if (s == "Speed") return TelemetryType::Speed;
    if (s == "Cadence") return TelemetryType::Cadence;
    if (s == "Temperature") return TelemetryType::Temperature;
    if (s == "Altitude") return TelemetryType::Altitude;
    if (s == "HeartRate") return TelemetryType::HeartRate;
    if (s == "Power") return TelemetryType::Power;
    if (s == "Distance") return TelemetryType::Distance;
    return TelemetryType::Undefined;
}

inline float GetTelemetryValue(const Telemetry& t, TelemetryType type) {
    switch (type) {
        case TelemetryType::Speed:        return t.speed;
        case TelemetryType::Cadence:      return t.cadence;
        case TelemetryType::Temperature:  return t.temperature;
        case TelemetryType::Altitude:     return t.altitude;
        case TelemetryType::HeartRate:    return t.heartrate;
        case TelemetryType::Power:        return t.power;
        case TelemetryType::Distance:     return t.distance;
        default: return 0.0f;
    }
}

class TelemetryDataProvider {
public:
    const Telemetry& get() const { return _data; }
    uint32_t version() const { return _version; }
    bool locationValid() const { return _data.validLocation; }

    void update(const Telemetry& newData) {
        _data = newData;
        ++_version;
    }

private:
    Telemetry _data{};
    uint32_t _version = 0;
};
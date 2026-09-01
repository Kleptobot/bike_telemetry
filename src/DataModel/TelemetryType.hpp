#ifndef TELEMETRYTYPE_H
#define TELEMETRYTYPE_H

#include <Arduino.h>

// The tile vocabulary: which quantity a layout cell displays. This is layout
// CONFIGURATION, not telemetry state -- it survives the removal of the
// Telemetry struct because screens/layouts/widgets still speak it.
enum class TelemetryType : uint8_t {
    Speed,
    Cadence,
    Temperature,
    Altitude,
    HeartRate,
    Power,
    Distance,
    TotalDist,
    Location,
    Grade,
    Undefined
};

inline TelemetryType& operator++(TelemetryType& t) {
    switch(t){
        case TelemetryType::Speed : t = TelemetryType::Cadence; break;
        case TelemetryType::Cadence : t = TelemetryType::Temperature; break;
        case TelemetryType::Temperature : t = TelemetryType::Altitude; break;
        case TelemetryType::Altitude : t = TelemetryType::HeartRate; break;
        case TelemetryType::HeartRate : t = TelemetryType::Power; break;
        case TelemetryType::Power : t = TelemetryType::Distance; break;
        case TelemetryType::Distance : t = TelemetryType::TotalDist; break;
        case TelemetryType::TotalDist : t = TelemetryType::Location; break;
        case TelemetryType::Location : t = TelemetryType::Grade; break;
        case TelemetryType::Grade: t = TelemetryType::Speed; break;
        default: t = TelemetryType::Undefined; break;
    }
    return t;
};

inline TelemetryType& operator--(TelemetryType& t) {
    switch(t){
        case TelemetryType::Speed : t = TelemetryType::Grade; break;
        case TelemetryType::Cadence : t = TelemetryType::Speed; break;
        case TelemetryType::Temperature : t = TelemetryType::Cadence; break;
        case TelemetryType::Altitude : t = TelemetryType::Temperature; break;
        case TelemetryType::HeartRate : t = TelemetryType::Altitude; break;
        case TelemetryType::Power : t = TelemetryType::HeartRate; break;
        case TelemetryType::Distance : t = TelemetryType::Power; break;
        case TelemetryType::TotalDist : t = TelemetryType::Distance; break;
        case TelemetryType::Location : t = TelemetryType::TotalDist; break;
        case TelemetryType::Grade: t = TelemetryType::Location; break;
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
        case TelemetryType::TotalDist : return"TotalDist"; break;
        case TelemetryType::Location : return"Location"; break;
        case TelemetryType::Grade : return"Grade"; break;
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
    if (s == "TotalDist") return TelemetryType::TotalDist;
    if (s == "Location") return TelemetryType::Location;
    if (s == "Grade") return TelemetryType::Grade;
    return TelemetryType::Undefined;
}

#endif /* TELEMETRYTYPE_H */

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
    TotalDist,
    Undefined
};

struct Telemetry {
    imu_data imu;
    dps_data dps;
    float speed;            //km/h
    float cadence;          //RPM
    float temperature;      //celcius
    float altitude;         //meters
    float heartrate;        //BPM
    float power;            //Watts
    bool validLocation;
    double longitude;       //degrees
    double latitude;        //degrees
    float distance;         //meters
    float totalDistance;    //meters

    Telemetry(){}
    Telemetry(
        const imu_data& imu_,
        const dps_data& dps_,
        float speed_,
        float cadence_,
        float temperature_,
        float altitude_,
        float heartrate_,
        float power_,
        bool validLocation_,
        double longitude_,
        double latitude_,
        float distance_
    )
        : imu(imu_)
        , dps(dps_)
        , speed(speed_)
        , cadence(cadence_)
        , temperature(temperature_)
        , altitude(altitude_)
        , heartrate(heartrate_)
        , power(power_)
        , validLocation(validLocation_)
        , longitude(longitude_)
        , latitude(latitude_)
        , distance(distance_)
        , totalDistance(distance_)
    {}

    Telemetry& operator=(const Telemetry& _new) {
        if (this == &_new) {
            return *this;
        }

        imu            = _new.imu;
        dps            = _new.dps;
        speed          = _new.speed;
        cadence        = _new.cadence;
        temperature    = _new.temperature;
        altitude       = _new.altitude;
        heartrate      = _new.heartrate;
        power          = _new.power;
        validLocation  = _new.validLocation;
        longitude      = _new.longitude;
        latitude       = _new.latitude;
        distance       = _new.distance;
        totalDistance  += _new.totalDistance;

        return *this;
    }
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
        case TelemetryType::TotalDist : t = TelemetryType::Speed; break;
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
        case TelemetryType::TotalDist : t = TelemetryType::Distance; break;
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
        case TelemetryType::TotalDist:    return t.totalDistance / 1000.0;  //convert to km
        default: return 0.0f;
    }
}

class TelemetryDataProvider {
public:
    const Telemetry& get() const { return _data; }
    uint32_t version() const { return _version; }

    void update(const Telemetry& newData) {
        _data = newData;
        ++_version;
    }

    void resetDistance() { _data.totalDistance = 0;}

private:
    Telemetry _data{};
    uint32_t _version = 0;
};
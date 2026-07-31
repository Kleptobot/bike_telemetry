#ifndef TELEMETRYDATAPROVIDER_H
#define TELEMETRYDATAPROVIDER_H

#include <Arduino.h>
#include <variant>

#include "HAL/SensorData.hpp"

#include <vector>

struct GPSPoint {
    double lat;
    double lon;
    uint32_t ts;
    GPSPoint() : lat(0), lon(0), ts(0) {}
    GPSPoint(double la, double lo, uint32_t t) : lat(la), lon(lo), ts(t) {}
};

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

struct Telemetry {
    imu_data imu;
    dps_data dps;
    int16_t BattPercentage;
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
    float grade;

    Telemetry(){}
    Telemetry(
        const imu_data& imu_,
        const dps_data& dps_,
        int16_t BattPercentage_,
        float speed_,
        float cadence_,
        float temperature_,
        float altitude_,
        float heartrate_,
        float power_,
        bool validLocation_,
        double longitude_,
        double latitude_,
        float distance_,
        float grade_
    )
        : imu(imu_)
        , dps(dps_)
        , BattPercentage(BattPercentage_)
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
        , grade(grade_)
    {}

    // Copy assignment is a plain copy. It previously did
    //     totalDistance += _new.totalDistance;
    // which hid a distance accumulator inside operator=, so assigning one
    // Telemetry to another was not idempotent: `a = b; a = b;` produced a
    // different result from `a = b`. It happened to give the right answer
    // because exactly one call site ever assigned a Telemetry. Accumulation
    // now lives in TelemetryDataProvider::update, where it is visible.
    Telemetry& operator=(const Telemetry&) = default;
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
        default: t = TelemetryType::Undefined;
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

inline std::variant<float, location_data> GetTelemetryValue(const Telemetry& t, TelemetryType type) {
    switch (type) {
        case TelemetryType::Speed:        return t.speed;
        case TelemetryType::Cadence:      return t.cadence;
        case TelemetryType::Temperature:  return t.temperature;
        case TelemetryType::Altitude:     return t.altitude;
        case TelemetryType::HeartRate:    return t.heartrate;
        case TelemetryType::Power:        return t.power;
        case TelemetryType::Distance:     return t.distance;
        case TelemetryType::TotalDist:    return t.totalDistance / 1000.0;  //convert to km
        case TelemetryType::Location:     return (location_data){t.validLocation, t.longitude, t.latitude};
        case TelemetryType::Grade:        return t.grade;
        default: return 0.0f;
    }
}

class TelemetryDataProvider {
public:
    const Telemetry& get() const { return _data; }
    uint32_t version() const { return _version; }

    void update(const Telemetry& newData) {
        // Carry the running total across, then add this tick's increment.
        // Telemetry's constructor seeds totalDistance from the per-tick
        // distance, so newData.totalDistance is the increment, not a total.
        // This accumulation used to be hidden inside Telemetry::operator=.
        const float carried = _data.totalDistance;

        _data = newData;
        _data.totalDistance = carried + newData.totalDistance;
        ++_version;

        // append gps point if valid
        if (_data.validLocation) {
            appendPoint(_data.latitude, _data.longitude, millis());
        }
    }

    void resetDistance() { _data.totalDistance = 0;}

    // Recent track access
    const std::vector<GPSPoint>& recentTrack() const { return _recent; }
    void clearTrack() { _recent.clear(); }

private:
    void appendPoint(double lat, double lon, uint32_t ts) {
        if (_recent.empty() || _recent.back().lat != lat || _recent.back().lon != lon) {
            _recent.emplace_back(lat, lon, ts);
            if (_recent.size() > _maxPoints) {
                // simple pop-front
                _recent.erase(_recent.begin());
            }
        }
    }
    
    Telemetry _data{};
    uint32_t _version = 0;
    std::vector<GPSPoint> _recent;
    const size_t _maxPoints = 600;
};

#endif
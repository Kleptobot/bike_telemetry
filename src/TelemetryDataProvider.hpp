#pragma once
#include <Arduino.h>

#include "HAL/SensorData.hpp"

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
};

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
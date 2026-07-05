#ifndef ALTITUDEFUSION_H
#define ALTITUDEFUSION_H

#include <Arduino.h>
#include <limits.h>

class AltitudeFusion {
    private:

    unsigned long last_micros;
    uint32_t lastGPSAltUpdate = (uint32_t)ULONG_MAX;
    bool altitudeValidLast;

    float est_alt = 0.0f;           // Cleaned Absolute Altitude (meters above sea level)
    float est_vel_z = 0.0f;         // Vertical speed / Climb rate (m/s)
    float acc_bias_z = 0.0f;        // Tracked accelerometer offset caused by vibration/
    const float k_baro = 0.05f;     // Smooths out wind gusts hitting the barometer casing
    const float k_gps  = 0.005f;    // Very slow correction to fix weather pressure drift
    const float k_bias = 0.001f;    // Slowly learns and removes steady accelerometer bias/tilt

    public:

    void altitudeIMUUpdate(float accZ);
    void altitudeDPSUpdate(float dpsAlt);
    void altitudeGPSCorrect(float gpsAlt);
    const float& altitude() const { return est_alt; };
    const float& rise() const { return est_vel_z; };

};

#endif /* ALTITUDEFUSION_H */
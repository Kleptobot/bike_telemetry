#ifndef ALTITUDEFUSION_H
#define ALTITUDEFUSION_H

#include <Arduino.h>
#include <limits.h>

static constexpr float STANDARD_GRAVITY = 9.80665f;
static constexpr float STANDARD_SEA_LEVEL_PA = 101325.0f;
static constexpr float BARO_LAPSE_RATE = 0.0065f;
static constexpr float BARO_EXPONENT = 0.1902663539f;

class AltitudeFusion {
    private:

    unsigned long last_micros = 0;
    uint32_t lastGPSAltUpdate = (uint32_t)ULONG_MAX;
    bool altitudeValidLast = false;

    float est_alt = 0.0f;
    float est_vel_z = 0.0f;
    float acc_bias_z = 0.0f;
    float baro_bias_z = 0.0f;       // NEW: slow bias GPS learns against baro's absolute output
    const float k_baro = 0.05f;
    const float k_gps  = 0.02f;     // raised now that it only trims a slow bias term
    const float k_bias = 0.001f;

    // -- P0 calibration state --
    float P0_local = STANDARD_SEA_LEVEL_PA;
    bool P0_calibrated = false;   // NEW: tracks whether we've ever locked P0
    float gps_alt_history[5] = {0};
    uint8_t gps_hist_count = 0;
    static constexpr float GPS_STABILITY_THRESHOLD_M = 3.0f; // max spread to trust a calibration
    static constexpr float P0_LOWPASS_ALPHA = 0.1f;          // slow-moving P0 update

    uint32_t lastP0UpdateMs = 0;
    static constexpr uint32_t P0_UPDATE_COOLDOWN_MS = 30000;

    float computeSeaLevelPressure(float pressurePa, float tempC, float knownAltM) const;
    bool gpsHistoryIsStable() const;

    public:

    void altitudeIMUUpdate(float accZ);
    void altitudeDPSUpdate(float dpsAlt);
    void altitudeGPSCorrect(float gpsAlt);

    // Call alongside altitudeGPSCorrect() with the raw pressure/temp
    // used to compute dpsAlt, so P0_local can be recalibrated.
    void baroCalibrateFromGPS(float gpsAlt, float pressurePa, float tempC);

    float computeAltitudeFromPressure(float pressurePa, float tempC) const;

    const float& altitude() const { return est_alt; };
    const float& rise() const { return est_vel_z; };
    const float& seaLevelPressure() const { return P0_local; };
    const bool& seaLevelPressureCalibrated() const { return P0_calibrated; };

};

#endif /* ALTITUDEFUSION_H */
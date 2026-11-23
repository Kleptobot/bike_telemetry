#ifndef ALTITUDEFUSION_H
#define ALTITUDEFUSION_H

#include <Arduino.h>
static constexpr float STANDARD_GRAVITY = 9.80665f;
static constexpr float STANDARD_SEA_LEVEL_PA = 101325.0f;
static constexpr float BARO_LAPSE_RATE = 0.0065f;
static constexpr float BARO_EXPONENT = 0.1902663539f;

class AltitudeFusion {
    private:

    bool altitudeValidLast = false;

    float est_alt = 0.0f;
    float est_vel_z = 0.0f;
    float baro_bias_z = 0.0f;       // Slow bias GPS learns against baro's absolute output
    const float k_baro = 0.05f;
    const float k_gps  = 0.02f;     // raised now that it only trims a slow bias term
    float lastBaroAltitudeM = 0.0f;
    bool hasLastBaroAltitude = false;

    // -- P0 calibration state --
    float P0_local = STANDARD_SEA_LEVEL_PA;
    bool P0_calibrated = false;
    float gps_alt_history[5] = {0};
    uint8_t gps_hist_count = 0;
    static constexpr float GPS_STABILITY_THRESHOLD_M = 3.0f; // max spread to trust a calibration
    static constexpr float P0_LOWPASS_ALPHA = 0.01f;

    float computeSeaLevelPressure(float pressurePa, float tempC, float knownAltM) const;
    bool gpsHistoryIsStable() const;
    float gpsHistoryMean() const;

    public:

    void altitudeDPSUpdate(float dpsAlt, float dtSeconds);
    void altitudeGPSCorrect(float gpsAlt);

    // Call alongside altitudeGPSCorrect() with the raw pressure/temp
    // used to compute dpsAlt, so P0_local can be calibrated and slowly tracked.
    void baroCalibrateFromGPS(float gpsAlt, float pressurePa, float tempC);

    float computeAltitudeFromPressure(float pressurePa, float tempC) const;

    const float& altitude() const { return est_alt; };
    const float& rise() const { return est_vel_z; };
    const float& seaLevelPressure() const { return P0_local; };
    const bool& seaLevelPressureCalibrated() const { return P0_calibrated; };

};

#endif /* ALTITUDEFUSION_H */
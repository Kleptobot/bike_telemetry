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

    unsigned long lastGPSAltUpdate = (uint32_t)ULONG_MAX;
    bool altitudeValidLast = false;

    float est_alt = 0.0f;
    float est_vel_z = 0.0f;
    float acc_bias_z = 0.0f;
    float baro_bias_z = 0.0f;       // NEW: slow bias GPS learns against baro's absolute output
    const float k_baro = 0.05f;
    const float k_gps  = 0.02f;     // raised now that it only trims a slow bias term
    const float k_bias = 0.001f;

    // Rejects a single GPS commit further than this from the current estimate
    // in the bias-trim branch: an indoor-multipath jump must not be absorbed
    // into baro_bias_z, from where it would drag the fused altitude for
    // minutes. The bootstrap and the pre-calibration re-anchor are NOT
    // outlier-gated -- without any absolute reference the filter output is
    // worse than a rough one.
    static constexpr float GPS_ALT_OUTLIER_M = 10.0f;

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
    // One entry into the rolling stability history (shift-left register).
    void pushGpsAltHistory(float gpsAlt);

    public:

    void altitudeIMUUpdate(float accZ, float dtSeconds);
    void altitudeDPSUpdate(float dpsAlt);
    // GPS altitude commit. fixQualityOk is the caller's fix-quality verdict
    // (the HDOP gate in FusionEngine); it is honoured ONLY by the bias-trim
    // branch -- the first-fix bootstrap and the pre-calibration re-anchor
    // accept whatever fix exists.
    void altitudeGPSCorrect(float gpsAlt, bool fixQualityOk);

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
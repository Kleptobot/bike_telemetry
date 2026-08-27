#include "AltitudeFusion.hpp"

void AltitudeFusion::altitudeIMUUpdate(float accZ) {
    unsigned long current_micros = micros();
    float dt = (current_micros - last_micros) / 1000000.0f;
    last_micros = current_micros;

    if (dt <= 0.0f || dt > 0.1f) dt = 0.01f; // Basic safety check

    // accZ arrives in g -- LSM6DS3::readFloatAccelZ() returns multiples of
    // standard gravity, not m/s^2 -- so it has to be converted before gravity
    // is subtracted. Previously this computed accZ - 9.80665, which at rest is
    // 1.0 - 9.81 = -8.81 m/s^2 of fictitious downward acceleration fed straight
    // into the velocity integrator. The barometer correction absorbed most of
    // it into acc_bias_z over time, which is why altitude still looked
    // reasonable, but est_vel_z -- and therefore rise(), and the grade tile
    // that divides by it -- was not trustworthy.
    float linear_acc_z = (accZ * STANDARD_GRAVITY) - STANDARD_GRAVITY - acc_bias_z;

    // Kinematic integration step
    est_alt += est_vel_z * dt;
    est_vel_z += linear_acc_z * dt;
}

void AltitudeFusion::altitudeDPSUpdate(float dpsAlt) {
    float corrected_baro = dpsAlt - baro_bias_z;
    float error = corrected_baro - est_alt;

    est_alt += error * k_baro;
    est_vel_z += error * (k_baro * 0.1f);
    acc_bias_z -= error * k_bias;
}

void AltitudeFusion::altitudeGPSCorrect(float gpsAlt) {
    if (!altitudeValidLast) {
        est_alt = gpsAlt;
        baro_bias_z = 0.0f;
        altitudeValidLast = true;
    } else {
        float gps_error = gpsAlt - est_alt;
        baro_bias_z -= gps_error * k_gps;
    }
}

float AltitudeFusion::computeAltitudeFromPressure(float pressurePa, float tempC) const {
    float Tb = 273.15f + tempC;
    float P_Pb = pow(pressurePa / P0_local, -BARO_EXPONENT);
    return (Tb * P_Pb - Tb) / (BARO_LAPSE_RATE * P_Pb);
}

float AltitudeFusion::computeSeaLevelPressure(float pressurePa, float tempC, float knownAltM) const {
    float Tb = 273.15f + tempC;
    float exponent = 1.0f / BARO_EXPONENT;
    float base = 1.0f - (BARO_LAPSE_RATE * knownAltM) / Tb;
    return pressurePa / powf(base, exponent);
}

bool AltitudeFusion::gpsHistoryIsStable() const {
    if (gps_hist_count < 5) return false;
    float minV = gps_alt_history[0], maxV = gps_alt_history[0];
    for (uint8_t i = 1; i < 5; i++) {
        minV = min(minV, gps_alt_history[i]);
        maxV = max(maxV, gps_alt_history[i]);
    }
    return (maxV - minV) <= GPS_STABILITY_THRESHOLD_M;
}

void AltitudeFusion::baroCalibrateFromGPS(float gpsAlt, float pressurePa, float tempC) {
    // Always feed the rolling history so the stability check reflects live
    // data, regardless of how often the caller happens to invoke this.
    for (uint8_t i = 0; i < 4; i++) gps_alt_history[i] = gps_alt_history[i + 1];
    gps_alt_history[4] = gpsAlt;
    if (gps_hist_count < 5) gps_hist_count++;

    if (!P0_calibrated) {
        // Bootstrap from a single sample rather than waiting on 5 stable
        // history entries -- altitudeDPSUpdate() is gated off until this
        // fires (see HAL.cpp), so getting *some* P0 quickly matters more
        // than getting a perfect one. A noisy first snap gets pulled toward
        // the true value by the slow-track branch below once the history
        // is stable.
        P0_local = computeSeaLevelPressure(pressurePa, tempC, gpsAlt);
        P0_calibrated = true;
        lastP0UpdateMs = millis();
        return;
    }

    if (!gpsHistoryIsStable()) return;
    if (millis() - lastP0UpdateMs < P0_UPDATE_COOLDOWN_MS) return;

    float impliedP0 = computeSeaLevelPressure(pressurePa, tempC, gpsAlt);
    P0_local += (impliedP0 - P0_local) * P0_LOWPASS_ALPHA;
    lastP0UpdateMs = millis();
}
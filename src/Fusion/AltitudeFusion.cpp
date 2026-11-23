#include "AltitudeFusion.hpp"
#include <cmath>

void AltitudeFusion::altitudeDPSUpdate(float dpsAlt, float dtSeconds) {
    float corrected_baro = dpsAlt - baro_bias_z;

    if (!altitudeValidLast) {
        est_alt = corrected_baro;
        altitudeValidLast = true;
    } else {
        float error = corrected_baro - est_alt;
        est_alt += error * k_baro;
    }

    // A one-axis accelerometer cannot distinguish vertical motion from tilt
    // and gravity projection. Use the pressure altitude trend for vario so
    // small static acceleration errors cannot integrate into runaway speed.
    if (hasLastBaroAltitude && dtSeconds > 0.0f && dtSeconds <= 5.0f) {
        float rawVario = (dpsAlt - lastBaroAltitudeM) / dtSeconds;
        rawVario = constrain(rawVario, -8.0f, 8.0f);
        const float alpha = dtSeconds / (2.0f + dtSeconds);
        est_vel_z += alpha * (rawVario - est_vel_z);
        if (fabsf(est_vel_z) < 0.05f) est_vel_z = 0.0f;
    } else {
        est_vel_z = 0.0f;
    }

    lastBaroAltitudeM = dpsAlt;
    hasLastBaroAltitude = true;
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

float AltitudeFusion::gpsHistoryMean() const {
    float sum = 0.0f;
    for (uint8_t i = 0; i < 5; i++) sum += gps_alt_history[i];
    return sum / 5.0f;
}

void AltitudeFusion::baroCalibrateFromGPS(float gpsAlt, float pressurePa, float tempC) {
    // Always feed the rolling history so the stability check reflects live
    // data, regardless of how often the caller happens to invoke this.
    for (uint8_t i = 0; i < 4; i++) gps_alt_history[i] = gps_alt_history[i + 1];
    gps_alt_history[4] = gpsAlt;
    if (gps_hist_count < 5) gps_hist_count++;

    if (!gpsHistoryIsStable()) return;

    const float impliedP0 = computeSeaLevelPressure(pressurePa, tempC,
                                                    gpsHistoryMean());
    if (!P0_calibrated) {
        P0_local = impliedP0;
        P0_calibrated = true;
    } else {
        // Called for each fresh, quality-gated GPS altitude. Tracking each
        // stable fix avoids a long cooldown leaving P0 stale for a whole ride.
        P0_local += (impliedP0 - P0_local) * P0_LOWPASS_ALPHA;
    }
}
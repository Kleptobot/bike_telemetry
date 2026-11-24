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

    if (!imuAltitudeValid) {
        imuPredictedAltM = est_alt;
        imuAltitudeValid = true;
    } else {
        // Barometric updates anchor the short IMU prediction. This is a slow
        // correction rather than a hard reset, avoiding a 1 Hz altitude step.
        imuPredictedAltM += (est_alt - imuPredictedAltM) * k_baro;
    }

    // Keep pressure as the absolute/slow vertical-rate reference. The IMU
    // state is updated independently between these barometer samples.
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

void AltitudeFusion::altitudeIMUUpdate(float accelXG, float accelYG,
                                       float accelZG, float dtSeconds) {
    if (dtSeconds <= 0.0f || dtSeconds > 1.0f) return;

    const float accelMagnitude = sqrtf(accelXG * accelXG +
                                       accelYG * accelYG +
                                       accelZG * accelZG);
    if (!std::isfinite(accelMagnitude) || accelMagnitude < 0.5f ||
        accelMagnitude > 1.5f) {
        return;
    }

    if (!imuGravityValid) {
        gravityX = accelXG;
        gravityY = accelYG;
        gravityZ = accelZG;
        imuGravityValid = true;
        imuVerticalAccelMs2 = 0.0f;
        imuVerticalVelMs = 0.0f;
        imuPredictedAltM = altitudeValidLast ? est_alt : 0.0f;
        imuAltitudeValid = altitudeValidLast;
        return;
    }

    const float gravityTau =
        1.0f / (2.0f * 3.14159265358979323846f * IMU_GRAVITY_CUTOFF_HZ);
    const float gravityAlpha = dtSeconds / (gravityTau + dtSeconds);
    gravityX += (accelXG - gravityX) * gravityAlpha;
    gravityY += (accelYG - gravityY) * gravityAlpha;
    gravityZ += (accelZG - gravityZ) * gravityAlpha;

    const float gravityMagnitude = sqrtf(gravityX * gravityX +
                                         gravityY * gravityY +
                                         gravityZ * gravityZ);
    if (gravityMagnitude < 0.5f) return;

    const float gravityUnitX = gravityX / gravityMagnitude;
    const float gravityUnitY = gravityY / gravityMagnitude;
    const float gravityUnitZ = gravityZ / gravityMagnitude;

    // The low-pass gravity estimate removes the mounting/DC contribution.
    // Projecting the residual specific force onto this direction gives
    // approximately linear acceleration along global vertical.
    const float residualX = accelXG - gravityX;
    const float residualY = accelYG - gravityY;
    const float residualZ = accelZG - gravityZ;
    const float rawVerticalAccelMs2 =
        -(residualX * gravityUnitX +
          residualY * gravityUnitY +
          residualZ * gravityUnitZ) * STANDARD_GRAVITY;

    const float accelTau =
        1.0f / (2.0f * 3.14159265358979323846f * IMU_ACCEL_CUTOFF_HZ);
    const float accelAlpha = dtSeconds / (accelTau + dtSeconds);
    imuVerticalAccelMs2 +=
        (rawVerticalAccelMs2 - imuVerticalAccelMs2) * accelAlpha;

    if (fabsf(imuVerticalAccelMs2) < 0.03f) imuVerticalAccelMs2 = 0.0f;

    imuVerticalVelMs += imuVerticalAccelMs2 * dtSeconds;

    // Integrators drift. Correct IMU velocity toward the independent
    // barometric vario estimate with a multi-second time constant.
    const float velocityCorrectionAlpha =
        dtSeconds / (IMU_VEL_CORRECTION_TAU_S + dtSeconds);
    imuVerticalVelMs +=
        (est_vel_z - imuVerticalVelMs) * velocityCorrectionAlpha;

    imuVerticalVelMs = constrain(imuVerticalVelMs, -8.0f, 8.0f);
    if (fabsf(imuVerticalVelMs) < 0.05f) imuVerticalVelMs = 0.0f;

    if (imuAltitudeValid) {
        imuPredictedAltM += imuVerticalVelMs * dtSeconds;
    } else if (altitudeValidLast) {
        imuPredictedAltM = est_alt;
        imuAltitudeValid = true;
    }

    // Keep the displayed vario responsive between one-second barometer updates.
    est_vel_z = 0.7f * imuVerticalVelMs + 0.3f * est_vel_z;
    if (fabsf(est_vel_z) < 0.05f) est_vel_z = 0.0f;
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
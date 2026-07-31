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

    // The core fusion check: update altitude and capture acceleration errors
    float error = dpsAlt - est_alt;
    
    est_alt += error * k_baro;
    est_vel_z += error * (k_baro * 0.1f);
    
    // This line absorbs long-term tilt errors or constant vibration biases
    acc_bias_z -= error * k_bias;
}

void AltitudeFusion::altitudeGPSCorrect(float gpsAlt) {
    // 1. Critical initialization step
    if (!altitudeValidLast) {
        // First time getting a valid lock, instantly snap to GPS height
        // This anchors your absolute baseline right away
        est_alt = gpsAlt;
        altitudeValidLast = true;
    } else {
        // 2. Continuous drift correction
        // Slowly nudge the filter toward the absolute GPS value
        float gps_error = gpsAlt - est_alt;
        est_alt += gps_error * k_gps;
    }
}
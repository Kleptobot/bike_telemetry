#include "AltitudeFusion.hpp"

void AltitudeFusion::altitudeIMUUpdate(float accZ) {
    unsigned long current_micros = micros();
    float dt = (current_micros - last_micros) / 1000000.0f;
    last_micros = current_micros;

    if (dt <= 0.0f || dt > 0.1f) dt = 0.01f; // Basic safety check

    // Subtract gravity AND our learned vibration bias
    float linear_acc_z = accZ - 9.80665f - acc_bias_z;

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
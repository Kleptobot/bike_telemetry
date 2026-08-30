#ifndef LSM6DS3_H_STUB
#define LSM6DS3_H_STUB
#include "Arduino.h"
#define I2C_MODE 0

// Status codes mirroring the real driver's status_t (Sensors.cpp compares
// against IMU_SUCCESS after the burst read).
typedef enum { IMU_SUCCESS, IMU_HW_ERROR } status_t;

// First register of the contiguous gyro+accel output block the firmware
// burst-reads (see Sensors.cpp).
#define LSM6DS3_ACC_GYRO_OUTX_L_G 0x22

class LSM6DS3 {
public:
    LSM6DS3(uint8_t = I2C_MODE, uint8_t = 0x6A) {}
    int16_t begin() { return 0; }

    // One deterministic sample per burst read, decoded by Sensors.cpp through
    // the same calc*() scaling the real driver applies. 1 g on Z at rest,
    // everything else zero -- the exact values the pre-burst readFloat*()
    // stubs returned, so simulator behaviour is unchanged by the refactor.
    status_t readRegisterRegion(uint8_t* buf, uint8_t offset, uint8_t len) {
        for (uint8_t i = 0; i < len; ++i) buf[i] = 0;
        if (offset == LSM6DS3_ACC_GYRO_OUTX_L_G && len >= 12) {
            buf[10] = 0x00;   // accel-Z raw = 0x4000 = 16384 LSB,
            buf[11] = 0x40;   // which is 1 g at the 2 g scale below
        }
        return IMU_SUCCESS;
    }
    float calcAccel(int16_t raw) const { return (float)raw / 16384.0f; }        // g (2 g scale)
    float calcGyro(int16_t raw)  const { return (float)raw * 8.75f / 1000.0f; } // dps (245 dps)
};
#endif

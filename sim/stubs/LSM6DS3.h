#ifndef LSM6DS3_H_STUB
#define LSM6DS3_H_STUB
#include "Arduino.h"
#define I2C_MODE 0
class LSM6DS3 {
public:
    LSM6DS3(uint8_t = I2C_MODE, uint8_t = 0x6A) {}
    int16_t begin() { return 0; }
    // Accelerometer values are in g, matching the real driver -- the
    // simulator must reproduce that, since mixing g and m/s^2 was a real bug.
    float readFloatAccelX() { return 0.0f; }
    float readFloatAccelY() { return 0.0f; }
    float readFloatAccelZ() { return 1.0f; }   // 1 g at rest
    float readFloatGyroX()  { return 0.0f; }
    float readFloatGyroY()  { return 0.0f; }
    float readFloatGyroZ()  { return 0.0f; }
};
#endif

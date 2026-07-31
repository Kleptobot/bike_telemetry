#ifndef ADAFRUIT_I2CDEVICE_H_STUB
#define ADAFRUIT_I2CDEVICE_H_STUB
#include "Arduino.h"
#include "Wire.h"
// Adafruit_GFX includes this but the canvas path never uses it.
class Adafruit_I2CDevice {
public:
    Adafruit_I2CDevice(uint8_t = 0, TwoWire* = nullptr) {}
    bool begin(bool = true) { return true; }
};
#endif

#ifndef ADAFRUIT_SPIDEVICE_H_STUB
#define ADAFRUIT_SPIDEVICE_H_STUB
#include "Arduino.h"
#include "SPI.h"
class Adafruit_SPIDevice {
public:
    Adafruit_SPIDevice(int8_t = -1, uint32_t = 1000000, uint8_t = MSBFIRST, uint8_t = SPI_MODE0, SPIClass* = nullptr) {}
    bool begin() { return true; }
};
#endif

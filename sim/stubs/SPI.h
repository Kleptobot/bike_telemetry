#ifndef SPI_H_STUB
#define SPI_H_STUB
#include "Arduino.h"
#define MSBFIRST 1
#define SPI_MODE0 0
#define SPI_MODE3 3
class SPISettings {
public:
    SPISettings() {}
    SPISettings(uint32_t, uint8_t, uint8_t) {}
};
class SPIClass {
public:
    void begin() {}
    void end() {}
    void beginTransaction(SPISettings) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t d) { return d; }
};
extern SPIClass SPI;
#endif

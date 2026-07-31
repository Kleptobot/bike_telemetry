#ifndef WIRE_H_STUB
#define WIRE_H_STUB
#include "Arduino.h"
// The LC76G driver is excluded from the simulator build (it is replaced
// wholesale by the simulated HAL), so this only needs to satisfy includes.
class TwoWire {
public:
    void begin() {}
    void end() {}
    void setClock(uint32_t) {}
    void beginTransmission(uint8_t) {}
    uint8_t endTransmission(bool = true) { return 0; }
    uint8_t requestFrom(uint8_t, size_t) { return 0; }
    size_t write(uint8_t) { return 1; }
    size_t write(const uint8_t*, size_t n) { return n; }
    int available() { return 0; }
    int read() { return -1; }
};
extern TwoWire Wire;
#endif

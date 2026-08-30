#ifndef DPS3XX_H_STUB
#define DPS3XX_H_STUB
#include "Arduino.h"
#include "Wire.h"

// Mirrors the real library's util/dps_config.h -- Sensors.cpp sizes its
// buffers with it, matching what getContResults() may legally write.
#define DPS__FIFO_SIZE 32

class Dps3xx {
public:
    void begin(TwoWire&) {}
    int16_t startMeasureBothCont(int16_t, int16_t, int16_t, int16_t) { return 0; }
    int16_t getContResults(float* temp, uint8_t& tempCount, float* prs, uint8_t& prsCount);
};
#endif

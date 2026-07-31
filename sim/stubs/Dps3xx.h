#ifndef DPS3XX_H_STUB
#define DPS3XX_H_STUB
#include "Arduino.h"
#include "Wire.h"
class Dps3xx {
public:
    void begin(TwoWire&) {}
    int16_t startMeasureBothCont(int16_t, int16_t, int16_t, int16_t) { return 0; }
    int16_t getContResults(float* temp, uint8_t& tempCount, float* prs, uint8_t& prsCount);
};
#endif

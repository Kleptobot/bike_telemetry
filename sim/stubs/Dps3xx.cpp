#include "Dps3xx.h"
// Simulated barometer. Values are driven by the scenario via these globals so
// altitude and the derived grade respond to a simulated climb.
static float g_simPressurePa  = 101325.0f;
static float g_simBaroTempC   = 21.5f;
void simSetPressurePa(float pa) { g_simPressurePa = pa; }
void simSetBaroTempC(float c)   { g_simBaroTempC = c; }

int16_t Dps3xx::getContResults(float* temp, uint8_t& tempCount, float* prs, uint8_t& prsCount) {
    // The firmware averages whatever it is given; one sample each is enough
    // and keeps the arithmetic identical.
    if (tempCount) { temp[0] = g_simBaroTempC; tempCount = 1; }
    if (prsCount)  { prs[0]  = g_simPressurePa; prsCount = 1; }
    return 0;
}

#include "Dps3xx.h"
// Simulated barometer. Values are driven by the scenario via these globals so
// altitude and the derived grade respond to a simulated climb.
static float g_simPressurePa  = 101325.0f;
static float g_simBaroTempC   = 21.5f;
static uint8_t g_simTemperatureCount = 1;
static uint8_t g_simPressureCount = 1;
static int16_t g_simDpsResultCode = 0;

void simSetPressurePa(float pa) { g_simPressurePa = pa; }
void simSetBaroTempC(float c)   { g_simBaroTempC = c; }
void simSetDpsResultCounts(uint8_t temperatureCount, uint8_t pressureCount,
                           int16_t resultCode) {
    g_simTemperatureCount = temperatureCount;
    g_simPressureCount = pressureCount;
    g_simDpsResultCode = resultCode;
}

int16_t Dps3xx::getContResults(float* temp, uint8_t& tempCount, float* prs, uint8_t& prsCount) {
    const uint8_t temperatureSamples =
        g_simTemperatureCount < tempCount ? g_simTemperatureCount : tempCount;
    const uint8_t pressureSamples =
        g_simPressureCount < prsCount ? g_simPressureCount : prsCount;

    for (uint8_t i = 0; i < temperatureSamples; ++i) {
        temp[i] = g_simBaroTempC;
    }
    for (uint8_t i = 0; i < pressureSamples; ++i) {
        prs[i] = g_simPressurePa;
    }
    tempCount = temperatureSamples;
    prsCount = pressureSamples;
    return g_simDpsResultCode;
}

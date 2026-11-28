// Standalone smoke test for barometer sample validation. Build and run from sim/:
//   g++ -std=gnu++17 -I stubs -I ..\src tests\SensorsDpsValidity_smoke.cpp ..\src\HAL\Sensors.cpp stubs\Dps3xx.cpp stubs\Arduino.cpp -o build\SensorsDpsValidity_smoke.exe
//   build\SensorsDpsValidity_smoke.exe

#include "HAL/Sensors.hpp"
#include "Dps3xx.h"

#include <cmath>
#include <cstdio>
#include <limits>

TwoWire Wire;

int main() {
    simSetMillis(1001);
    simSetPressurePa(101325.0f);
    simSetBaroTempC(21.5f);
    simSetDpsResultCounts(1, 1, 0);

    SensorSystem sensors;
    sensors.update(false);
    const dps_data initial = sensors.dps();
    const uint16_t initialSeq = sensors.dpsSeq();
    const uint32_t initialTimestamp = sensors.dpsTsMs();
    int failures = 0;

    if (!initial.dpsValid || initialSeq != 1 ||
        !std::isfinite(initial.f32_DSP_Pa) ||
        !std::isfinite(initial.f32_DSP_Temp)) {
        std::printf("FAIL: valid pressure/temperature sample was rejected\n");
        failures++;
    }

    auto rejectNextResult = [&](uint8_t temperatureCount,
                                uint8_t pressureCount,
                                int16_t resultCode,
                                float pressurePa) {
        simAdvanceMillis(1001);
        simSetPressurePa(pressurePa);
        simSetDpsResultCounts(temperatureCount, pressureCount, resultCode);
        sensors.update(false);

        const dps_data current = sensors.dps();
        if (current.dpsValid || sensors.dpsSeq() != initialSeq ||
            sensors.dpsTsMs() != initialTimestamp ||
            !std::isfinite(current.f32_DSP_Pa) ||
            !std::isfinite(current.f32_DSP_Temp) ||
            current.f32_DSP_Pa != initial.f32_DSP_Pa ||
            current.f32_DSP_Temp != initial.f32_DSP_Temp) {
            std::printf("FAIL: invalid barometer result changed accepted sample state\n");
            failures++;
        }
    };

    rejectNextResult(0, 0, 0, 101325.0f);
    rejectNextResult(1, 0, 0, 101325.0f);
    rejectNextResult(1, 1, -1, 101325.0f);
    rejectNextResult(1, 1, 0, std::numeric_limits<float>::quiet_NaN());

    simAdvanceMillis(1001);
    simSetPressurePa(100000.0f);
    simSetDpsResultCounts(1, 1, 0);
    sensors.update(false);
    const dps_data recovered = sensors.dps();
    if (!recovered.dpsValid || sensors.dpsSeq() != initialSeq + 1 ||
        recovered.f32_DSP_Pa != 100000.0f) {
        std::printf("FAIL: valid barometer sample was not accepted after rejection\n");
        failures++;
    }

    std::printf(failures == 0 ? "SENSOR DPS VALIDITY SMOKE PASS\n"
                              : "SENSOR DPS VALIDITY SMOKE FAIL (%d)\n",
                failures);
    return failures == 0 ? 0 : 1;
}

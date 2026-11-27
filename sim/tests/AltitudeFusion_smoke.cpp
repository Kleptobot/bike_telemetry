// Standalone smoke test for AltitudeFusion. Build and run from sim/:
//   g++ -std=gnu++17 -I . -I stubs -I ..\src tests\AltitudeFusion_smoke.cpp ..\src\Fusion\AltitudeFusion.cpp stubs\Arduino.cpp -o build\AltitudeFusion_smoke.exe
//
// Covers stationary vario, barometric-rate response, and stable P0 calibration.

#include "Fusion/AltitudeFusion.hpp"
#include <cmath>
#include <cstdio>

static bool near(float actual, float expected, float tolerance) {
    return fabsf(actual - expected) <= tolerance;
}

static void calibrateAt(AltitudeFusion& fusion, float altitudeM,
                        float pressurePa) {
    for (int i = 0; i < 5; i++)
        fusion.baroCalibrateFromGPS(altitudeM, pressurePa, 20.0f);
}

int main() {
    int failures = 0;

    {
        AltitudeFusion fusion;
        calibrateAt(fusion, 100.0f, 100000.0f);
        fusion.altitudeDPSUpdate(100.0f, 1.0f);

        for (int i = 0; i < 30; i++)
            fusion.altitudeIMUUpdate(0.26f, -0.05f, -0.92f, 0.1f);

        if (fabsf(fusion.imuVerticalAcceleration()) > 0.05f ||
            fabsf(fusion.imuVerticalVelocity()) > 0.05f ||
            fabsf(fusion.altitude() - 100.0f) > 0.05f) {
            std::printf("FAIL: stationary IMU moved vertical estimate: "
                        "acc=%.3f vel=%.3f alt=%.3f\n",
                        fusion.imuVerticalAcceleration(),
                        fusion.imuVerticalVelocity(),
                        fusion.altitude());
            failures++;
        }
    }

    {
        AltitudeFusion fusion;
        calibrateAt(fusion, 100.0f, 100000.0f);
        fusion.altitudeDPSUpdate(100.0f, 1.0f);
        fusion.altitudeIMUUpdate(0.0f, 0.0f, -1.0f, 0.1f);
        fusion.altitudeIMUUpdate(0.0f, 0.0f, -0.9f, 0.1f);

        if (fusion.imuVerticalAcceleration() <= 0.0f) {
            std::printf("FAIL: positive upward acceleration had the wrong sign: %.3f m/s2\n",
                        fusion.imuVerticalAcceleration());
            failures++;
        }
    }

    {
        AltitudeFusion fusion;
        calibrateAt(fusion, 100.0f, 100000.0f);
        for (int i = 0; i < 30; i++)
            fusion.altitudeDPSUpdate(100.0f - 0.02f * i, 1.0f);

        if (fabsf(fusion.rise()) > 0.05f) {
            std::printf("FAIL: stationary vario drifted to %.3f m/s\n",
                        fusion.rise());
            failures++;
        }
    }

    {
        AltitudeFusion fusion;
        calibrateAt(fusion, 100.0f, 100000.0f);
        fusion.altitudeDPSUpdate(100.0f, 1.0f);
        for (int i = 1; i <= 20; i++)
            fusion.altitudeDPSUpdate(100.0f + i, 1.0f);

        if (!near(fusion.rise(), 1.0f, 0.05f)) {
            std::printf("FAIL: steady 1 m/s barometric trend gave %.3f m/s\n",
                        fusion.rise());
            failures++;
        }
    }

    {
        AltitudeFusion fusion;
        calibrateAt(fusion, 100.0f, 100000.0f);
        fusion.altitudeDPSUpdate(100.0f, 1.0f);
        fusion.altitudeDPSUpdate(101.0f, 2.0f);

        if (!near(fusion.rise(), 0.25f, 0.01f)) {
            std::printf("FAIL: 2-second sample interval gave %.3f m/s\n",
                        fusion.rise());
            failures++;
        }
    }

    {
        AltitudeFusion fusion;
        const float unstableAltitudes[] = {100.0f, 106.0f, 100.0f, 106.0f, 100.0f};
        for (float altitudeM : unstableAltitudes)
            fusion.baroCalibrateFromGPS(altitudeM, 100000.0f, 20.0f);
        if (fusion.seaLevelPressureCalibrated()) {
            std::printf("FAIL: P0 calibrated from unstable GPS history\n");
            failures++;
        }

        calibrateAt(fusion, 100.0f, 100000.0f);
        if (!fusion.seaLevelPressureCalibrated()) {
            std::printf("FAIL: P0 did not calibrate after stable GPS history\n");
            failures++;
        } else {
            const float initialP0 = fusion.seaLevelPressure();
            for (int i = 0; i < 20; i++)
                fusion.baroCalibrateFromGPS(100.0f, 100100.0f, 20.0f);

            if (fusion.seaLevelPressure() <= initialP0 + 5.0f) {
                std::printf("FAIL: stable P0 tracking did not respond: %.2f -> %.2f Pa\n",
                            initialP0, fusion.seaLevelPressure());
                failures++;
            }
        }
    }

    std::printf(failures == 0 ? "ALTITUDE FUSION SMOKE PASS\n"
                              : "ALTITUDE FUSION SMOKE FAIL (%d)\n",
                failures);
    return failures == 0 ? 0 : 1;
}

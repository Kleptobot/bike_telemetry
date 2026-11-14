// Standalone smoke test for AltitudeFusion -- the "altitude slowly drifts
// down while the device sits on a bench" regressions.
//
// Three failure modes are covered, each a real report against the filter:
//   1. Marginal bench fix (HDOP >= 2): P0 never calibrates, so the barometer
//      never feeds the filter and est_alt becomes pure IMU dead-reckoning --
//      any accelerometer scale/offset error integrates into an unbounded
//      drift. Fixed by re-anchoring est_alt to every GPS commit while
//      uncalibrated; this test proves the anchor holds and that the control
//      (no commits) does drift.
//   2. Calibrated bench with a drifting, poor-quality GPS fix: the bias-trim
//      branch used to accept every commit at k_gps=0.02 (~50 s time
//      constant), dragging the fused altitude down with GPS wander. Fixed by
//      the fix-quality gate + outlier rejection.
//   3. P0 slow-track "stability" history was fed at fix rate (~1 Hz), so its
//      5-sample window spanned five seconds and every slow drift passed. It
//      is now paced by the 30 s cooldown (~2.5 min window), which rejects the
//      drift rates that used to walk P0 -- and baro altitude -- downward.
//
// Build and run (from sim/):
//   g++ -Wall -Wextra -std=gnu++17 -DARDUINO=10800 -Istubs -I../src
//       tests/AltitudeFusion_smoke.cpp ../src/Fusion/AltitudeFusion.cpp
//       stubs/Arduino.cpp -o build/obike-altfusion-smoke
//   ./build/obike-altfusion-smoke

#include "Fusion/AltitudeFusion.hpp"
#include <cstdio>
#include <cmath>

static int failures = 0;
static void check(bool cond, const char* what, float got = 0.0f, float want = 0.0f) {
    if (cond) return;
    std::printf("[AltFusion smoke] FAIL: %s (got %.3f want %.3f)\n", what, got, want);
    ++failures;
}

// Pressure at `altM` under the same standard-atmosphere model the filter
// uses, so bootstrap P0 comes out at exactly STANDARD_SEA_LEVEL_PA and every
// scenario starts from a known-good calibration.
static float pressureAt(float altM, float tempC) {
    const float Tb = 273.15f + tempC;
    const float base = 1.0f - (BARO_LAPSE_RATE * altM) / Tb;
    return STANDARD_SEA_LEVEL_PA * powf(base, 1.0f / BARO_EXPONENT);
}

int main() {
    // -------------------------------------------------------------------------
    // Scenario 1: uncalibrated filter + drifting accelerometer + GPS re-anchor
    // (bench, marginal fix). The re-anchor must hold est_alt at the GPS value
    // no matter how hard the IMU channel is pushed.
    // -------------------------------------------------------------------------
    {
        AltitudeFusion af;
        // Bootstrap with a poor-quality fix: fixQualityOk = false, P0 stays
        // uncalibrated exactly as on a bench with HDOP >= 2.
        af.altitudeGPSCorrect(150.0f, false);
        check(af.seaLevelPressureCalibrated() == false, "s1: P0 still uncalibrated");

        // 60 s at 10 Hz with accZ = 0.98 (2% scale error -> linear ~ -0.196
        // m/s^2). A GPS altitude commit every second (fixQualityOk = false,
        // 0.2 m of noise). Without the re-anchor this control case falls
        // hundreds of metres; with it, est_alt must stay pinned.
        AltitudeFusion control;          // same IMU feed, NO GPS commits
        control.altitudeGPSCorrect(150.0f, false);
        for (int i = 0; i < 600; i++) {
            af.altitudeIMUUpdate(0.98f, 0.1f);
            control.altitudeIMUUpdate(0.98f, 0.1f);
            if ((i % 10) == 9) {
                const float noise = ((i / 10) % 2) ? 0.2f : -0.2f;
                af.altitudeGPSCorrect(150.0f + noise, false);
            }
        }
        std::printf("s1 anchored: est_alt=%.2f control=%.2f\n",
                    af.altitude(), control.altitude());
        check(std::fabs(af.altitude() - 150.0f) < 1.0f,
              "s1: re-anchor holds est_alt", af.altitude(), 150.0f);
        check(control.altitude() < 100.0f,
              "s1 control: IMU dead-reckoning does drift without commits",
              control.altitude(), 0.0f);
    }

    // -------------------------------------------------------------------------
    // Scenario 2: calibrated filter, constant pressure, poor-quality GPS
    // drifting 20 m down over 10 minutes (the original bench report).
    // -------------------------------------------------------------------------
    {
        const float tempC = 20.0f;
        const float P100  = pressureAt(100.0f, tempC);

        AltitudeFusion af;
        af.baroCalibrateFromGPS(100.0f, P100, tempC);
        check(af.seaLevelPressureCalibrated() == true, "s2: P0 calibrated");

        float gpsAlt = 100.0f;
        for (int s = 0; s < 600; s++) {          // one simulated second per pass
            af.altitudeIMUUpdate(1.0f, 0.1f);    // accelerometer exactly at rest
            const float baroAlt = af.computeAltitudeFromPressure(P100, tempC);
            af.altitudeDPSUpdate(baroAlt);
            gpsAlt -= 20.0f / 600.0f;            // GPS wanders 20 m down
            af.altitudeGPSCorrect(gpsAlt, false); // HDOP >= 2: trim must skip
        }
        std::printf("s2 poor-fix drift: est_alt=%.2f baro=%.2f\n",
                    af.altitude(), af.computeAltitudeFromPressure(P100, tempC));
        check(std::fabs(af.altitude() - 100.0f) < 1.5f,
              "s2: poor-quality drift does not drag fused altitude",
              af.altitude(), 100.0f);
    }

    // -------------------------------------------------------------------------
    // Scenario 3: calibrated trim branch -- outlier rejected, inlier trims.
    // -------------------------------------------------------------------------
    {
        const float tempC = 20.0f;
        const float P100  = pressureAt(100.0f, tempC);

        AltitudeFusion af;
        af.baroCalibrateFromGPS(100.0f, P100, tempC);
        af.altitudeGPSCorrect(100.0f, true);      // bootstrap est_alt (and arm
                                                  // the trim branch) at 100 m
        for (int i = 0; i < 10; i++) {
            af.altitudeIMUUpdate(1.0f, 0.1f);
            af.altitudeDPSUpdate(af.computeAltitudeFromPressure(P100, tempC));
        }
        const float before = af.altitude();
        check(std::fabs(before - 100.0f) < 1.0f,
              "s3: settled near 100 m before the trim tests", before, 100.0f);

        af.altitudeGPSCorrect(50.0f, true);       // 50 m multipath jump
        check(std::fabs(af.altitude() - before) < 0.01f,
              "s3: outlier commit rejected outright", af.altitude(), before);

        af.altitudeGPSCorrect(102.0f, true);      // inlier: +2 m, within gate
        af.altitudeDPSUpdate(af.computeAltitudeFromPressure(P100, tempC));
        std::printf("s3 inlier trim: before=%.3f after=%.3f\n", before, af.altitude());
        check(af.altitude() > before + 0.001f,
              "s3: inlier commit still trims baro bias", af.altitude(), before);
    }

    // -------------------------------------------------------------------------
    // Scenario 4: P0 slow-track pacing. Bootstrap at 100 m, then GPS altitude
    // drifts 3 m/min for 10 minutes. The cooldown-paced history samples (30 s
    // apart) span 6 m > 3 m threshold, so every window is rejected and P0
    // never moves. The old fix-rate-fed history spanned ~0.15 m per window,
    // passed as "stable", and walked P0 down.
    // -------------------------------------------------------------------------
    {
        const float tempC = 20.0f;
        const float P100  = pressureAt(100.0f, tempC);

        AltitudeFusion af;
        af.baroCalibrateFromGPS(100.0f, P100, tempC);
        const float p0AfterBootstrap = af.seaLevelPressure();
        check(std::fabs(p0AfterBootstrap - STANDARD_SEA_LEVEL_PA) < 1.0f,
              "s4: bootstrap P0 is standard sea level", p0AfterBootstrap,
              STANDARD_SEA_LEVEL_PA);

        float gpsAlt = 100.0f;
        for (int s = 0; s < 600; s++) {
            delay(1000);                          // advance simulated millis
            gpsAlt -= 3.0f / 60.0f;               // 3 m/min downward drift
            af.baroCalibrateFromGPS(gpsAlt, P100, tempC);
        }
        std::printf("s4 paced history: P0=%.1f gpsAlt=%.1f\n",
                    af.seaLevelPressure(), gpsAlt);
        check(std::fabs(af.seaLevelPressure() - STANDARD_SEA_LEVEL_PA) < 5.0f,
              "s4: slow-track rejects drifting GPS history", af.seaLevelPressure(),
              STANDARD_SEA_LEVEL_PA);
    }

    std::printf("[AltFusion smoke] %s (%d failures)\n",
                failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}

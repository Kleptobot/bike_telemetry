// Standalone smoke test for FitnessFusion. Not part of the Makefile: built
// ad hoc on the host to validate the accumulator math without the full App
// (the headless sim currently crashes after SD mount in this environment for
// reasons unrelated to this module).
#include "Fusion/FitnessFusion.hpp"
#include <cstdio>
#include <cmath>

int main() {
    int failures = 0;

    // --- Scenario: power meter live at 180 W, FTP 200 for 60 s ---------------
    {
        FitnessFusion ff;
        FitnessProfile p;
        p.ftpWatts = 200;
        for (int i = 0; i < 60; i++) {
            MeasurementFrame f;
            f.heartRateBpm.value = 132.0f;
            f.heartRateBpm.live  = true;
            f.rtcNow.value       = 1700000000 + i;
            ff.update(f, p, 1.0f, 180.0f, 0.0f);
        }
        const FitnessOutput& o = ff.out();
        // 180 W x 60 s = 10.8 kJ mechanical; / (0.24 eff x 4.184) = ~10.75 kcal
        const float kcal = 10.8f / (0.24f * 4.184f);
        std::printf("180W scenario: kcal=%.2f(want %.2f) np=%.1f(want 180.0) "
                    "if=%.3f(want 0.900) tss=%.3f(want ~1.35) hrZone=%u(want 2) "
                    "pwrZone=%u(want 4) z2sec=%.1f(want 60)\n",
                    o.caloriesKcal, kcal, o.normalizedPowerW, o.intensityFactor,
                    o.tss, o.hrZone, o.powerZone, o.timeInZoneSec[1]);
        if (std::fabs(o.caloriesKcal - kcal) > 1.0f) failures++;
        if (std::fabs(o.normalizedPowerW - 180.0f) > 5.0f) failures++;
        if (std::fabs(o.intensityFactor - 0.9f) > 0.02f) failures++;
        if (std::fabs(o.tss - 1.35f) > 0.3f) failures++;
        if (o.hrZone != 2) failures++;      // 132 bpm: >=117 (z2), <138 (z3)
        if (o.powerZone != 4) failures++;   // 180/200 = 0.90 -> threshold zone 4
        if (o.timeInZoneSec[1] < 59.0f) failures++;
    }

    // --- Scenario: no power meter, HR only -> HR calorie path -----------------
    {
        FitnessFusion ff;
        FitnessProfile p;
        p.ftpWatts = 200;
        p.caloricProfile = CaloricProfile::Other;
        p.ageYears = 30.0f;
        p.riderMassKg = 75.0f;
        for (int i = 0; i < 10; i++) {
            MeasurementFrame f;
            f.heartRateBpm.value = 132.0f;
            f.heartRateBpm.live  = true;
            f.rtcNow.value       = 1700000000 + i;
            ff.update(f, p, 1.0f, 0.0f, 0.0f);
        }
        std::printf("HR-only scenario: kcal(10s)=%.3f hrZone=%u np=%.1f if=%.3f tss=%.3f\n",
                    ff.out().caloriesKcal, ff.out().hrZone, ff.out().normalizedPowerW,
                    ff.out().intensityFactor, ff.out().tss);
        if (ff.out().caloriesKcal <= 0.0f) failures++;
        if (ff.out().normalizedPowerW != 0.0f) failures++;  // no meter -> no NP
        if (ff.out().hrZone != 2) failures++;
    }

    // --- Scenario: est-power fallback (no meter, no HR) -----------------------
    {
        FitnessFusion ff;
        FitnessProfile p;
        for (int i = 0; i < 30; i++) {
            MeasurementFrame f;
            f.rtcNow.value = 1700000000 + i;
            ff.update(f, p, 1.0f, 0.0f, 200.0f);   // physics estimate 200 W
        }
        std::printf("est-power scenario: kcal(30s)=%.2f (want ~5.37) pwrZone=%u\n",
                    ff.out().caloriesKcal, ff.out().powerZone);
        if (ff.out().caloriesKcal <= 0.0f) failures++;
    }

    // --- Exercise the device uses... calories persist across reset() ---------
    {
        FitnessFusion ff;
        MeasurementFrame f;
        f.heartRateBpm.value = 150.0f;
        f.heartRateBpm.live  = true;
        f.rtcNow.value       = 1700000000;
        ff.update(f, FitnessProfile{}, 1.0f, 100.0f, 0.0f);
        ff.reset();
        if (ff.out().caloriesKcal != 0.0f) failures++;
        if (ff.out().tss != 0.0f) failures++;
    }

    std::printf(failures == 0 ? "SMOKE PASS\n" : "SMOKE FAIL (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}
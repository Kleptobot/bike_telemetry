// Standalone smoke test for FtpEstimator. Validates the rolling 20-minute
// window and the x0.95 FTP suggestion without the full App.
#include "Fusion/FtpEstimator.hpp"
#include <cstdio>

int main() {
    int failures = 0;

    // --- Scenario 1: steady 200 W for 25 minutes ---------------------------
    {
        FtpEstimator est;
        for (int i = 0; i < 25 * 60; i++) est.addSample(200);
        std::printf("200W steady: best20min=%u(want 200) suggestedFtp=%u(want 190) qualifying=%u(want 1)\n",
                    est.best20MinAvg(), est.suggestedFtp(), est.hasQualifyingWindow());
        if (est.best20MinAvg() != 200) failures++;
        if (est.suggestedFtp() != 190) failures++;
        if (!est.hasQualifyingWindow()) failures++;
    }

    // --- Scenario 2: only 10 minutes (no qualifying window) ----------------
    {
        FtpEstimator est;
        for (int i = 0; i < 10 * 60; i++) est.addSample(300);
        std::printf("10min only: best20min=%u(want 0) suggestedFtp=%u(want 0) qualifying=%u(want 0)\n",
                    est.best20MinAvg(), est.suggestedFtp(), est.hasQualifyingWindow());
        if (est.best20MinAvg() != 0) failures++;
        if (est.suggestedFtp() != 0) failures++;
        if (est.hasQualifyingWindow()) failures++;
    }

    // --- Scenario 3: ramping effort, best window in the middle --------------
    {
        FtpEstimator est;
        for (int i = 0; i < 5 * 60; i++) est.addSample(100);
        for (int i = 0; i < 20 * 60; i++) est.addSample(250);
        for (int i = 0; i < 5 * 60; i++) est.addSample(100);
        std::printf("ramp: best20min=%u(want 250) suggestedFtp=%u(want 237)\n",
                    est.best20MinAvg(), est.suggestedFtp());
        if (est.best20MinAvg() != 250) failures++;
        if (est.suggestedFtp() != 237) failures++;
    }

    // --- Scenario 4: reset clears state ------------------------------------
    {
        FtpEstimator est;
        for (int i = 0; i < 25 * 60; i++) est.addSample(200);
        est.reset();
        if (est.best20MinAvg() != 0) failures++;
        if (est.sampleCount() != 0) failures++;
        std::printf("reset: best20min=%u(want 0) count=%u(want 0)\n",
                    est.best20MinAvg(), est.sampleCount());
    }

    // --- Scenario 5: precision of x0.95 ------------------------------------
    {
        FtpEstimator est;
        for (int i = 0; i < 20 * 60; i++) est.addSample(210);
        std::printf("210W: suggestedFtp=%u(want 199)\n", est.suggestedFtp());
        if (est.suggestedFtp() != 199) failures++;
    }

    std::printf(failures == 0 ? "FTP SMOKE PASS\n" : "FTP SMOKE FAIL (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}
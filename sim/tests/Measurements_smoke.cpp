// Standalone smoke test for GPS sample freshness tracking. Build and run from sim/:
//   g++ -std=gnu++17 -I . -I stubs -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\TinyGPSPlus\src tests\Measurements_smoke.cpp -o build\Measurements_smoke.exe

#include "HAL/Measurements.hpp"
#include <cstdio>

int main() {
    AgeSeqTracker tracker;
    int failures = 0;

    if (tracker.note(0xFFFFFFFFu, 1000) || tracker.seq() != 0) {
        std::printf("FAIL: invalid GPS age produced a sample\n");
        failures++;
    }

    if (!tracker.note(0, 1010) || tracker.seq() != 1) {
        std::printf("FAIL: first GPS commit was not detected\n");
        failures++;
    }

    if (tracker.note(100, 1110) || tracker.seq() != 1) {
        std::printf("FAIL: increasing age was treated as a new commit\n");
        failures++;
    }

    if (!tracker.note(0, 2010) || tracker.seq() != 2 ||
        tracker.dtMs() != 1000) {
        std::printf("FAIL: second GPS commit was not detected with its interval\n");
        failures++;
    }

    if (tracker.note(0xFFFFFFFFu, 2020) ||
        !tracker.note(5, 4010) || tracker.seq() != 3 ||
        tracker.dtMs() != 2000) {
        std::printf("FAIL: GPS reacquisition after an invalid age was not tracked\n");
        failures++;
    }

    std::printf(failures == 0 ? "MEASUREMENTS SMOKE PASS\n"
                              : "MEASUREMENTS SMOKE FAIL (%d)\n",
                failures);
    return failures == 0 ? 0 : 1;
}

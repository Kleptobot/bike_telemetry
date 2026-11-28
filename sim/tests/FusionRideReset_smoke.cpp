// Standalone smoke test for ride-scoped fusion accumulator resets. Build and
// run from sim/:
//   g++ -std=gnu++17 -I stubs -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\TinyGPSPlus\src tests\FusionRideReset_smoke.cpp ..\src\Fusion\Fusion.cpp ..\src\Fusion\AltitudeFusion.cpp ..\src\Fusion\FitnessFusion.cpp -o build\FusionRideReset_smoke.exe
//   build\FusionRideReset_smoke.exe

#include "Fusion/Fusion.hpp"

#include <cmath>
#include <cstdio>

int main() {
    FusionEngine fusion;
    MeasurementFrame frame{};
    const FitnessProfile profile{};

    frame.powerWatts = {250.0f, true};
    frame.rtcNow.tsMs = 1000;
    fusion.update(frame, 0, profile);

    frame.rtcNow.tsMs = 2000;
    fusion.update(frame, 0, profile);
    frame.rtcNow.tsMs = 3000;
    fusion.update(frame, 0, profile);
    if (std::fabs(fusion.out().energyKj - 0.5f) > 0.001f) {
        std::printf("FAIL: energy did not accumulate before the ride reset\n");
        return 1;
    }

    fusion.resetDistance();
    frame.rtcNow.tsMs = 4000;
    fusion.update(frame, 0, profile);
    if (std::fabs(fusion.out().energyKj - 0.25f) > 0.001f) {
        std::printf("FAIL: ride reset did not restart energy accumulation\n");
        return 1;
    }

    std::printf("PASS: ride energy starts fresh after resetDistance()\n");
    return 0;
}

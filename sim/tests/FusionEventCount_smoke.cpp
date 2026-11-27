// Standalone smoke test for CSC event-count fusion. Build and run from sim/:
//   g++ -std=gnu++17 -I stubs -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\TinyGPSPlus\src tests\FusionEventCount_smoke.cpp ..\src\Fusion\Fusion.cpp ..\src\Fusion\AltitudeFusion.cpp ..\src\Fusion\FitnessFusion.cpp -o build\FusionEventCount_smoke.exe

#include "Fusion/Fusion.hpp"
#include <cmath>
#include <cstdio>

static void setEvent(EventCount& event, uint32_t counter, uint16_t eventTime,
                     uint32_t sampleMs, uint32_t sourceId) {
    event.counter = counter;
    event.evtTime1024 = eventTime;
    event.valid = true;
    event.tsMs = sampleMs;
    event.seq = (uint16_t)counter;
    event.sourceId = sourceId;
}

int main() {
    int failures = 0;
    FusionEngine fusion;
    const FitnessProfile profile{};
    MeasurementFrame frame{};
    frame.tickStartMs = 1000;
    setEvent(frame.wheelRevs, 0xFFFFFFFEu, 65000, 1000, 1);
    setEvent(frame.crankRevs, 0xFFFFFFFEu, 65000, 1000, 2);
    fusion.update(frame, 2100, profile);
    if (!fusion.out().wheelRevsValid || !fusion.out().crankRevsValid ||
        fusion.out().wheelRpm != 0.0f || fusion.out().cadenceRpm != 0.0f) {
        std::printf("FAIL: first event packet was not used as a baseline\n");
        failures++;
    }

    frame.tickStartMs = 2000;
    setEvent(frame.wheelRevs, 0xFFFFFFFFu, (uint16_t)(65000u + 1024u), 2000, 1);
    setEvent(frame.crankRevs, 0xFFFFFFFFu, (uint16_t)(65000u + 1024u), 2000, 2);
    fusion.update(frame, 2100, profile);
    if (std::fabs(fusion.out().wheelRpm - 0.6f) > 0.01f ||
        std::fabs(fusion.out().cadenceRpm - 0.6f) > 0.01f) {
        std::printf("FAIL: filtered output did not respond to the first 60 RPM event delta\n");
        failures++;
    }

    // A ten-second revolution interval is a legitimate slowing event, not a
    // dropout. Both the revolution and event-time counters wrap in this step.
    frame.tickStartMs = 12000;
    setEvent(frame.wheelRevs, 0u, (uint16_t)(488u + 10240u), 12000, 1);
    setEvent(frame.crankRevs, 0u, (uint16_t)(488u + 10240u), 12000, 2);
    fusion.update(frame, 2100, profile);
    if (!fusion.out().wheelRpmValid || !fusion.out().cadenceValid ||
        fusion.out().wheelRpm <= 0.6f || fusion.out().wheelRpm >= 1.0f ||
        fusion.out().cadenceRpm <= 0.6f || fusion.out().cadenceRpm >= 1.0f) {
        std::printf("FAIL: late, wrapped event deltas were not handled smoothly\n");
        failures++;
    }

    // A new primary sensor must establish its own baseline; its counter must
    // not be subtracted from the previous sensor's cumulative count.
    frame.tickStartMs = 13000;
    setEvent(frame.wheelRevs, 100u, 15000u, 13001u, 3);
    fusion.update(frame, 2100, profile);
    if (!fusion.out().wheelRevsValid || fusion.out().wheelRpm != 0.0f) {
        std::printf("FAIL: changing the wheel source did not rebaseline safely\n");
        failures++;
    }

    frame.tickStartMs = 36001;
    frame.gpsSpeedKmh.valid = true;
    frame.gpsSpeedKmh.value = 18.5;
    fusion.update(frame, 2100, profile);
    if (fusion.out().crankRevsValid || fusion.out().cadenceValid ||
        !fusion.out().wheelRpmValid) {
        std::printf("FAIL: crank and wheel freshness limits were not independent\n");
        failures++;
    }

    frame.tickStartMs = 53002;
    fusion.update(frame, 2100, profile);
    if (fusion.out().wheelRevsValid || fusion.out().wheelRpmValid ||
        fusion.out().wheelRpm != 0.0f ||
        std::fabs(fusion.out().speedKmh - 18.5f) > 0.01f) {
        std::printf("FAIL: stale wheel data was not cleared or GPS fallback did not resume\n");
        failures++;
    }

    // Both distance estimates are retained; the legacy output prefers a
    // configured live wheel sensor and falls back to GPS.
    FusionEngine distanceFusion;
    MeasurementFrame distanceFrame{};
    distanceFrame.tickStartMs = 1000;
    distanceFrame.gpsPos = {0.0, 0.0, true, 1000, 1, 0};
    setEvent(distanceFrame.wheelRevs, 100u, 1000u, 1000u, 7u);
    distanceFusion.update(distanceFrame, 2100, profile);

    distanceFrame.tickStartMs = 2000;
    distanceFrame.gpsPos = {0.0, 0.001, true, 2000, 2, 1000};
    setEvent(distanceFrame.wheelRevs, 102u, 3048u, 2000u, 7u);
    distanceFusion.update(distanceFrame, 2100, profile);
    if (std::fabs(distanceFusion.out().wheelDistanceDeltaM - 4.2f) > 0.01f ||
        std::fabs(distanceFusion.out().gpsDistanceDeltaM - 111.2f) > 1.0f ||
        std::fabs(distanceFusion.out().distanceDeltaM - 4.2f) > 0.01f ||
        std::fabs(distanceFusion.out().wheelTotalDistanceM - 4.2f) > 0.01f ||
        std::fabs(distanceFusion.out().gpsTotalDistanceM - 111.2f) > 1.0f) {
        std::printf("FAIL: independent GPS/wheel distance estimates were incorrect\n");
        failures++;
    }

    // A changed sensor establishes its own counter baseline.
    distanceFrame.tickStartMs = 3000;
    setEvent(distanceFrame.wheelRevs, 10u, 1000u, 3000u, 8u);
    distanceFusion.update(distanceFrame, 2100, profile);
    if (distanceFusion.out().wheelDistanceDeltaM != 0.0f ||
        distanceFusion.out().distanceDeltaM != 0.0f) {
        std::printf("FAIL: changing the wheel source created a distance jump\n");
        failures++;
    }

    // Counter wrap is a small positive revolution delta, not a reset jump.
    distanceFrame.tickStartMs = 4000;
    setEvent(distanceFrame.wheelRevs, 0xFFFFFFFEu, 2000u, 4000u, 9u);
    distanceFusion.update(distanceFrame, 2100, profile);
    distanceFrame.tickStartMs = 5000;
    setEvent(distanceFrame.wheelRevs, 1u, 5072u, 5000u, 9u);
    distanceFusion.update(distanceFrame, 2100, profile);
    if (std::fabs(distanceFusion.out().wheelDistanceDeltaM - 6.3f) > 0.01f ||
        std::fabs(distanceFusion.out().wheelTotalDistanceM - 10.5f) > 0.01f) {
        std::printf("FAIL: wrapped wheel counter produced an incorrect distance\n");
        failures++;
    }

    // When the wheel source becomes unavailable, the preferred ride distance
    // switches to the next GPS displacement without mixing both increments.
    distanceFrame.tickStartMs = 6000;
    distanceFrame.gpsPos = {0.0, 0.002, true, 6000, 3, 1000};
    distanceFrame.wheelRevs.valid = false;
    distanceFusion.update(distanceFrame, 2100, profile);
    if (std::fabs(distanceFusion.out().gpsDistanceDeltaM - 111.2f) > 1.0f ||
        distanceFusion.out().wheelDistanceDeltaM != 0.0f ||
        std::fabs(distanceFusion.out().distanceDeltaM - 111.2f) > 1.0f) {
        std::printf("FAIL: GPS distance did not take over when wheel data became unavailable\n");
        failures++;
    }

    std::printf(failures == 0 ? "SMOKE PASS\n" : "SMOKE FAIL (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}

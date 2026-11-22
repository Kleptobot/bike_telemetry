// Standalone smoke test for TcxLogParser. Built ad hoc on the host:
//   g++ -std=gnu++17 -I . -I stubs -I host -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\ArduinoJson\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\TinyGPSPlus\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\LibYxml -I "..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\Adafruit GFX Library" tests\TcxLogParser_smoke.cpp ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\LibYxml\yxml.c stubs\Arduino.cpp stubs\SdFat.cpp -o build\TcxLogParser_smoke.exe
//
// Run from sim/ with a sample TCX in sdcard/.
// Validates that parsing a real TCX (written by TCXLogger) yields non-zero,
//
// sensible RideStats — the fix for the "all zeros + Error" LogSummaryScreen bug.

#include "Loggers/LogParser.hpp"
#include "stubs/SdFat.h"
#include <cstdio>
#include <cstring>

// Minimal IStorage backed by the host File32 stub.
class HostStorage : public IStorage {
public:
    File32 openFile(const char* path, oflag_t mode) override {
        File32 f;
        f.open(path, mode);
        return f;
    }
    bool exists(const char* path) override {
        return SdFat32().exists(path);
    }
    bool remove(const char* path) override {
        return SdFat32().remove(path);
    }
    bool listFiles(const char*, StorageEntry*, uint8_t, uint8_t& outCount) override {
        outCount = 0;
        return true;
    }
};

int main() {
    int failures = 0;

    // Mount the sim SD card so File32 can resolve paths.
    SdFat32 sd;
    sd.begin();

    HostStorage storage;
    TcxLogParser parser(&storage);

    if (!parser.open(&storage, "/20260101_120000.tcx")) {
        std::printf("FAIL: could not open /20260101_120000.tcx\n");
        return 1;
    }

    // The parser consumes a bounded chunk per step.
    while (parser.step()) {}
    if (!parser.done()) {
        std::printf("FAIL: done() is false after parsing\n");
        failures++;
    }

    const RideStats& s = parser.stats();
    std::printf("durationSec=%u distanceM=%.1f lapCount=%u\n",
                s.durationSec, s.distanceM, s.lapCount);
    std::printf("avgSpeedKmh=%.1f maxSpeedKmh=%.1f\n",
                s.avgSpeedKmh, s.maxSpeedKmh);
    std::printf("avgHr=%u maxHr=%u avgPower=%u maxPower=%u avgCadence=%u\n",
                s.avgHr, s.maxHr, s.avgPower, s.maxPower, s.avgCadence);
    std::printf("calories=%.1f np=%.1f tss=%.1f valid=%d\n",
                s.calories, s.np, s.tss, (int)s.valid);

    if (!s.valid) { std::printf("FAIL: stats not valid\n"); failures++; }
    if (s.durationSec != 15) { std::printf("FAIL: durationSec=%u want 15\n", s.durationSec); failures++; }
    if (s.distanceM < 240.0f || s.distanceM > 260.0f) {
        std::printf("FAIL: distanceM=%.1f want ~250\n", s.distanceM); failures++;
    }
    if (s.lapCount != 1) { std::printf("FAIL: lapCount=%u want 1\n", s.lapCount); failures++; }
    if (s.maxSpeedKmh < 55.0f || s.maxSpeedKmh > 65.0f) {
        std::printf("FAIL: maxSpeedKmh=%.1f want ~60\n", s.maxSpeedKmh); failures++;
    }
    if (s.avgHr < 135 || s.avgHr > 145) {
        std::printf("FAIL: avgHr=%u want ~141\n", s.avgHr); failures++;
    }
    if (s.maxHr != 150) { std::printf("FAIL: maxHr=%u want 150\n", s.maxHr); failures++; }
    if (s.maxPower < 185 || s.maxPower > 195) {
        std::printf("FAIL: maxPower=%u want ~190\n", s.maxPower); failures++;
    }
    if (s.avgPower < 175 || s.avgPower > 185) {
        std::printf("FAIL: avgPower=%u want ~180\n", s.avgPower); failures++;
    }
    if (s.calories < 45.0f || s.calories > 55.0f) {
        std::printf("FAIL: calories=%.1f want ~50\n", s.calories); failures++;
    }
    if (s.np < 175.0f || s.np > 185.0f) {
        std::printf("FAIL: np=%.1f want ~180\n", s.np); failures++;
    }
    if (s.tss < 20.0f || s.tss > 30.0f) {
        std::printf("FAIL: tss=%.1f want ~25\n", s.tss); failures++;
    }

    const char* fallbackPath = "/tcx_duration_fallback.tcx";
    File32 fallbackFile = storage.openFile(
        fallbackPath, O_WRITE | O_CREAT | O_TRUNC);
    if (!fallbackFile.isOpen()) {
        std::printf("FAIL: could not create duration fallback fixture\n");
        failures++;
    } else {
        fallbackFile.print(
            "<TrainingCenterDatabase><Activities><Activity><Lap>"
            "<TotalTimeSeconds>0</TotalTimeSeconds>"
            "<Track><Trackpoint><Time>2026-01-01T12:00:00</Time>"
            "</Trackpoint><Trackpoint><Time>2026-01-01T12:00:15</Time>"
            "</Trackpoint></Track></Lap></Activity></Activities>"
            "</TrainingCenterDatabase>");
        fallbackFile.close();

        TcxLogParser fallbackParser(&storage);
        if (!fallbackParser.open(&storage, fallbackPath)) {
            std::printf("FAIL: could not open duration fallback fixture\n");
            failures++;
        } else {
            while (fallbackParser.step()) {}
            if (fallbackParser.stats().durationSec != 15) {
                std::printf("FAIL: timestamp duration=%u want 15\n",
                            fallbackParser.stats().durationSec);
                failures++;
            }
            fallbackParser.close();
        }
        if (!storage.remove(fallbackPath)) {
            std::printf("FAIL: could not remove duration fallback fixture\n");
            failures++;
        }
    }

    std::printf(failures == 0 ? "SMOKE PASS\n" : "SMOKE FAIL (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}
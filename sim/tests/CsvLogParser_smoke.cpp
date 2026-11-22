// Standalone smoke test for CsvLogParser. Build and run from sim/:
//   g++ -std=gnu++17 -I . -I stubs -I host -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\LibYxml tests\CsvLogParser_smoke.cpp ..\src\Loggers\CsvLogParser.cpp ..\src\Loggers\CSVLogger.cpp ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\LibYxml\yxml.c stubs\Arduino.cpp stubs\SdFat.cpp -o build\CsvLogParser_smoke.exe
//
// Exercises a CSVLogger-generated file and a legacy file without a summary
// footer. Both are parsed incrementally with fixed-size line storage.

#include "Loggers/CSVLogger.hpp"
#include "Loggers/CsvLogParser.hpp"
#include "stubs/SdFat.h"
#include <cstdio>

class HostStorage : public IStorage {
public:
    File32 openFile(const char* path, oflag_t mode) override {
        File32 file;
        file.open(path, mode);
        return file;
    }

    bool exists(const char* path) override {
        return SdFat32().exists(path);
    }

    bool remove(const char* path) override {
        return SdFat32().remove(path);
    }

    bool listFiles(const char*, StorageEntry*, uint8_t,
                   uint8_t& outCount) override {
        outCount = 0;
        return true;
    }
};

static bool near(float actual, float expected) {
    return actual >= expected - 0.05f && actual <= expected + 0.05f;
}

int main() {
    SdFat32 sd;
    sd.begin();
    HostStorage storage;
    int failures = 0;

    const timeData start(2026, 1, 1, 12, 0, 0);
    CSVLogger logger(&storage);
    logger.startLogging(start);

    Trackpoint point{};
    point.speed = 20;
    point.heartrate = 120;
    point.cadence = 80;
    point.power = 100;
    point.calories = 0;
    point.normalizedPower = 100;
    point.tss = 0;
    point.distance = 0;
    logger.addTrackpoint(point, start);

    logger.newLap(timeData(2026, 1, 1, 12, 0, 1));
    point.speed = 30;
    point.heartrate = 140;
    point.cadence = 90;
    point.power = 200;
    point.calories = 10;
    point.normalizedPower = 180;
    point.tss = 1;
    point.distance = 5.6f;
    logger.addTrackpoint(point, timeData(2026, 1, 1, 12, 0, 1));

    point.speed = 25;
    point.heartrate = 160;
    point.cadence = 100;
    point.power = 300;
    point.calories = 25;
    point.normalizedPower = 190;
    point.tss = 2;
    point.distance = 12.5f;
    logger.addTrackpoint(point, timeData(2026, 1, 1, 12, 0, 2));
    logger.finaliseLogging();

    const char* generatedPath = "/20260101_120000.csv";
    CsvLogParser parser(&storage);
    if (!parser.open(&storage, generatedPath)) {
        std::printf("FAIL: could not open CSVLogger output\n");
        return 1;
    }
    while (parser.step()) {}

    const RideStats& stats = parser.stats();
    if (!parser.done() || !stats.valid) {
        std::printf("FAIL: generated CSV did not parse\n");
        failures++;
    }
    if (stats.durationSec != 2 || !near(stats.distanceM, 12.5f) ||
        stats.lapCount != 2 || !near(stats.avgSpeedKmh, 22.5f)) {
        std::printf("FAIL: duration/distance/laps/avg speed=%u/%.2f/%u/%.2f\n",
                    stats.durationSec, stats.distanceM, stats.lapCount,
                    stats.avgSpeedKmh);
        failures++;
    }
    if (!near(stats.maxSpeedKmh, 30) || stats.avgHr != 140 ||
        stats.maxHr != 160 || stats.avgPower != 200 || stats.maxPower != 300 ||
        stats.avgCadence != 90 || !near(stats.calories, 25) ||
        !near(stats.np, 190) || !near(stats.tss, 2)) {
        std::printf("FAIL: generated CSV metric values are incorrect\n");
        failures++;
    }
    parser.close();
    if (!storage.remove(generatedPath)) {
        std::printf("FAIL: could not remove generated CSV\n");
        failures++;
    }

    const char* legacyPath = "/legacy_csv_smoke.csv";
    File32 legacyFile = storage.openFile(
        legacyPath, O_WRITE | O_CREAT | O_TRUNC);
    if (!legacyFile.isOpen()) {
        std::printf("FAIL: could not create legacy CSV fixture\n");
        failures++;
    } else {
        legacyFile.println(
            "time,latitude,longitude,altitude,speed,heartrate,cadence,power,"
            "calories,normalizedpower,intensityfactor,tss,hrzone,powerzone");
        legacyFile.println(
            "2026-01-01T12:00:00Z,0,0,0,20,120,80,100,0,100,0.5,0,0,0");
        legacyFile.println(
            "2026-01-01T12:00:02Z,0,0,0,30,140,100,200,10,180,0.6,1,0,0");
        legacyFile.close();

        CsvLogParser legacyParser(&storage);
        if (!legacyParser.open(&storage, legacyPath)) {
            std::printf("FAIL: could not open legacy CSV fixture\n");
            failures++;
        } else {
            while (legacyParser.step()) {}
            const RideStats& legacyStats = legacyParser.stats();
            if (!legacyStats.valid || legacyStats.durationSec != 2 ||
                legacyStats.lapCount != 0 || !near(legacyStats.avgSpeedKmh, 25) ||
                legacyStats.avgHr != 130 || legacyStats.avgPower != 150 ||
                legacyStats.avgCadence != 90) {
                std::printf("FAIL: legacy CSV fallback metrics are incorrect\n");
                failures++;
            }
            legacyParser.close();
        }
        if (!storage.remove(legacyPath)) {
            std::printf("FAIL: could not remove legacy CSV fixture\n");
            failures++;
        }
    }

    std::printf(failures == 0 ? "SMOKE PASS\n" : "SMOKE FAIL (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}

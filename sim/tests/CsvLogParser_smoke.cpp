// Standalone smoke test for CsvLogParser. Build and run from sim/:
//   g++ -std=gnu++17 -I . -I stubs -I host -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\LibYxml tests\CsvLogParser_smoke.cpp ..\src\Loggers\CsvLogParser.cpp ..\src\Loggers\CSVLogger.cpp ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\LibYxml\yxml.c stubs\Arduino.cpp stubs\SdFat.cpp -o build\CsvLogParser_smoke.exe
//
// Exercises a CSVLogger-generated file and a legacy file without a summary
// footer. Both are parsed incrementally with fixed-size line storage.

#include "Loggers/CSVLogger.hpp"
#include "Loggers/CsvLogParser.hpp"
#include "HAL/SensorData.hpp"
#include "stubs/SdFat.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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

static void publishAdditionalSignals(DataBus& bus) {
    bus.publish(Topic::Vario, -0.75f);
    bus.publish(Topic::SmoothedGrade, 3.2f);
    bus.publish(Topic::GearRatio, 2.0f);
    bus.publish(Topic::Energy, 4.5f);
    bus.publish(Topic::Acceleration, -0.2f);
    bus.publish(Topic::EstimatedPower, 87.0f);
    bus.publish(Topic::TotalAscent, 34.0f);
    bus.publish(Topic::TotalDescent, 5.0f);
    bus.publish(Topic::Coasting, true);
    bus.publish(Topic::ElapsedSec, 5.0f);
    bus.publish(Topic::MovingSec, 4.0f);
    bus.publish(Topic::Avg3sPower, 125.0f);
    bus.publish(Topic::WattsPerKg, 1.7f);
    bus.publish(Topic::Vam, 1200.0f);
    bus.publish(Topic::BatteryCharging, true);
    bus.publish(Topic::GpsQuality, (uint8_t)1);
    bus.publish(Topic::ShiftCount, (uint32_t)7);
    bus.publish(Topic::GearShifted, true);
    bus.publish(Topic::DistanceDeltaM, 0.75f);
    bus.publish(Topic::GpsSpeed, 20.5f);
    bus.publish(Topic::GpsAltitude, 101.5f);
    bus.publish(Topic::GpsCourse, 271.5f);
    bus.publish(Topic::GpsSats, (uint8_t)8);
    bus.publish(Topic::GpsHdop, 0.8f);
    bus.publish(Topic::BaroAlt, 99.3f);
    bus.publish(Topic::ImuValid, true);
    bus.publish(Topic::ImuAccelX, 0.1f);
    bus.publish(Topic::ImuAccelY, 0.2f);
    bus.publish(Topic::ImuAccelZ, 1.0f);
    bus.publish(Topic::ImuGyroX, 10.0f);
    bus.publish(Topic::ImuGyroY, 20.0f);
    bus.publish(Topic::ImuGyroZ, 30.0f);
    bus.publish(Topic::PedalBalance, 55.0f);
    bus.publish(Topic::TorqueNm, 31.0f);
    bus.publish(Topic::Battery, (int16_t)67);
    bus.publish(Topic::BatteryVolts, 3.85f);
    bus.publish(Topic::Temperature, 22.5f);
}

static void publishTrackpointSignals(DataBus& bus, const Trackpoint& point) {
    location_data location{};
    location.valid = true;
    location.latitude = 47.6;
    location.longitude = -122.3;
    bus.publish(Topic::Location, location);
    bus.publish(Topic::FusedAltitude, point.altitude);
    bus.publish(Topic::SelectedSpeed, point.speed);
    bus.publish(Topic::HeartRate, point.heartrate);
    bus.publish(Topic::Cadence, point.cadence);
    bus.publish(Topic::PowerMeter, point.power);
    bus.publish(Topic::Calories, point.calories);
    bus.publish(Topic::NormalizedPower, point.normalizedPower);
    bus.publish(Topic::IntensityFactor, point.intensityFactor);
    bus.publish(Topic::Tss, point.tss);
    bus.publish(Topic::HrZone, point.hrZone);
    bus.publish(Topic::PowerZone, point.powerZone);
    bus.publish(Topic::GpsValid, location.valid);
    bus.publish(Topic::TotalDistanceM, point.distance);
}

static void splitFields(const std::string& line,
                        std::vector<std::string>& fields) {
    size_t start = 0;
    while (true) {
        const size_t comma = line.find(',', start);
        std::string field = line.substr(start, comma - start);
        if (!field.empty() && field.back() == '\r') field.pop_back();
        fields.push_back(field);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
}

static bool nearColumn(const std::vector<std::string>& header,
                       const std::vector<std::string>& row,
                       const char* name, float expected) {
    for (size_t i = 0; i < header.size(); ++i) {
        if (header[i] == name) {
            if (i >= row.size()) return false;
            const float actual = (float)strtof(row[i].c_str(), nullptr);
            if (!near(actual, expected))
                std::printf("FAIL: CSV column %s=%s, expected %.2f\n",
                            name, row[i].c_str(), expected);
            return near(actual, expected);
        }
    }
    std::printf("FAIL: CSV header is missing column %s\n", name);
    return false;
}

int main() {
    SdFat32 sd;
    sd.begin();
    HostStorage storage;
    int failures = 0;

    if (topicFromDisplayName(String("Speed")) != Topic::SelectedSpeed ||
        topicFromDisplayName(String("TotalDist")) != Topic::TotalDistanceM) {
        std::printf("FAIL: existing layout names did not resolve to DataBus topics\n");
        failures++;
    }

    const timeData start(2026, 1, 1, 12, 0, 0);
    DataBus bus;
    publishAdditionalSignals(bus);
    CSVLogger logger(&storage, bus);
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
    publishTrackpointSignals(bus, point);
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
    publishTrackpointSignals(bus, point);
    logger.addTrackpoint(point, timeData(2026, 1, 1, 12, 0, 1));

    point.speed = 25;
    point.heartrate = 160;
    point.cadence = 100;
    point.power = 300;
    point.calories = 25;
    point.normalizedPower = 190;
    point.tss = 2;
    point.distance = 12.5f;
    publishTrackpointSignals(bus, point);
    logger.addTrackpoint(point, timeData(2026, 1, 1, 12, 0, 2));
    logger.finaliseLogging();

    const char* generatedPath = "/20260101_120000.csv";
    File32 generatedFile = storage.openFile(generatedPath, O_READ);
    if (!generatedFile.isOpen()) {
        std::printf("FAIL: could not read generated CSV\n");
        return 1;
    }
    std::string generated;
    char readBuffer[256];
    int bytesRead;
    while ((bytesRead = generatedFile.read(readBuffer, sizeof(readBuffer))) > 0)
        generated.append(readBuffer, (size_t)bytesRead);
    generatedFile.close();

    const size_t headerEnd = generated.find('\n');
    const size_t rowEnd = headerEnd == std::string::npos
                              ? std::string::npos
                              : generated.find('\n', headerEnd + 1);
    std::vector<std::string> header, firstRow;
    if (headerEnd == std::string::npos || rowEnd == std::string::npos) {
        std::printf("FAIL: generated CSV is missing its header or data row\n");
        failures++;
    } else {
        splitFields(generated.substr(0, headerEnd), header);
        splitFields(generated.substr(headerEnd + 1, rowEnd - headerEnd - 1), firstRow);
        if (header.size() != 54 || firstRow.size() != header.size()) {
            std::printf("FAIL: generated CSV column counts are %u/%u, expected 54\n",
                        (unsigned)header.size(), (unsigned)firstRow.size());
            failures++;
        }
        if (header.size() == 54 &&
            (header[0] != "time" || header[1] != "latitude" ||
             header[2] != "longitude" || header[3] != "altitude" ||
             header.back() != "locationvalid")) {
            std::printf("FAIL: generated CSV changed the established column order\n");
            failures++;
        }
        if (!nearColumn(header, firstRow, "vario", -0.75f) ||
            !nearColumn(header, firstRow, "shiftcount", 7.0f) ||
            !nearColumn(header, firstRow, "gpsvalid", 1.0f) ||
            !nearColumn(header, firstRow, "imuaccelx", 0.1f) ||
            !nearColumn(header, firstRow, "locationvalid", 1.0f)) {
            std::printf("FAIL: generated CSV did not preserve representative bus signals\n");
            failures++;
        }
    }

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

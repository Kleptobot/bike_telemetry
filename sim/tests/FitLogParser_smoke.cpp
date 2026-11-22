// Standalone smoke test for FitLogParser. Build and run from sim/:
//   g++ -std=gnu++17 -I . -I stubs -I host -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\LibYxml tests\FitLogParser_smoke.cpp ..\src\Loggers\FITWriter.cpp ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\LibYxml\yxml.c stubs\Arduino.cpp stubs\SdFat.cpp -o build\FitLogParser_smoke.exe
//
// The fixture is emitted with FitWriter, then decoded through the same
// streaming parser used by LogSummaryScreen.

#include "Loggers/FITWriter.hpp"
#include "Loggers/LogParser.hpp"
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

int main() {
    SdFat32 sd;
    sd.begin();
    HostStorage storage;
    const char* path = "/fit_parser_smoke.fit";
    int failures = 0;

    FitWriter writer(&storage);
    if (!writer.open(path)) {
        std::printf("FAIL: could not create FIT fixture\n");
        return 1;
    }

    const FitFieldDef recordFields[] = {
        {253, 4, FitBaseType::UINT32}, {5, 4, FitBaseType::UINT32},
        {6, 2, FitBaseType::UINT16}, {3, 1, FitBaseType::UINT8},
        {4, 1, FitBaseType::UINT8}, {7, 2, FitBaseType::UINT16},
        {15, 2, FitBaseType::UINT16},
    };
    const FitFieldDef lapFields[] = {
        {253, 4, FitBaseType::UINT32}, {2, 4, FitBaseType::UINT32},
        {7, 4, FitBaseType::UINT32}, {9, 4, FitBaseType::UINT32},
        {13, 2, FitBaseType::UINT16}, {15, 1, FitBaseType::UINT8},
        {16, 1, FitBaseType::UINT8}, {17, 1, FitBaseType::UINT8},
        {32, 4, FitBaseType::UINT32},
    };
    const FitFieldDef sessionFields[] = {
        {253, 4, FitBaseType::UINT32}, {2, 4, FitBaseType::UINT32},
        {7, 4, FitBaseType::UINT32}, {8, 4, FitBaseType::UINT32},
        {9, 4, FitBaseType::UINT32}, {5, 1, FitBaseType::ENUM},
        {26, 2, FitBaseType::UINT16}, {27, 2, FitBaseType::UINT16},
    };

    writer.writeDefinition(1, FitMesg::RECORD, recordFields, 7);
    writer.writeDefinition(2, FitMesg::LAP, lapFields, 9);
    writer.writeDefinition(3, FitMesg::SESSION, sessionFields, 8);

    writer.beginDataMessage(1);
    writer.writeU32(10); writer.writeU32(0); writer.writeU16(10000);
    writer.writeU8(130); writer.writeU8(80); writer.writeU16(170);
    writer.writeU16(100);
    writer.beginDataMessage(1);
    writer.writeU32(20); writer.writeU32(10000); writer.writeU16(12000);
    writer.writeU8(150); writer.writeU8(90); writer.writeU16(190);
    writer.writeU16(500);

    writer.beginDataMessage(2);
    writer.writeU32(20); writer.writeU32(10); writer.writeU32(10000);
    writer.writeU32(10000); writer.writeU16(12000);
    writer.writeU8(140); writer.writeU8(150); writer.writeU8(85);
    writer.writeU32(500);

    writer.beginDataMessage(3);
    writer.writeU32(20); writer.writeU32(10); writer.writeU32(10000);
    writer.writeU32(9000); writer.writeU32(10000); writer.writeU8(2);
    writer.writeU16(180); writer.writeU16(25);
    writer.close();

    FitLogParser parser(&storage);
    if (!parser.open(&storage, path)) {
        std::printf("FAIL: could not open FIT fixture\n");
        storage.remove(path);
        return 1;
    }
    while (parser.step()) {}

    const RideStats& stats = parser.stats();
    if (!parser.done()) { std::printf("FAIL: parser did not finish\n"); failures++; }
    if (!stats.valid) { std::printf("FAIL: stats are invalid\n"); failures++; }
    if (stats.durationSec != 9) {
        std::printf("FAIL: duration=%u want 9\n", stats.durationSec);
        failures++;
    }
    if (stats.distanceM < 99.9f || stats.distanceM > 100.1f) {
        std::printf("FAIL: distance=%.1f want 100\n", stats.distanceM);
        failures++;
    }
    if (stats.lapCount != 1) {
        std::printf("FAIL: laps=%u want 1\n", stats.lapCount);
        failures++;
    }
    if (stats.avgSpeedKmh < 39.9f || stats.avgSpeedKmh > 40.1f) {
        std::printf("FAIL: average speed=%.1f want 40\n", stats.avgSpeedKmh);
        failures++;
    }
    if (stats.maxSpeedKmh < 43.1f || stats.maxSpeedKmh > 43.3f) {
        std::printf("FAIL: max speed=%.1f want 43.2\n", stats.maxSpeedKmh);
        failures++;
    }
    if (stats.avgHr != 140 || stats.maxHr != 150) {
        std::printf("FAIL: heart rate=%u/%u want 140/150\n",
                    stats.avgHr, stats.maxHr);
        failures++;
    }
    if (stats.avgPower != 180 || stats.maxPower != 190) {
        std::printf("FAIL: power=%u/%u want 180/190\n",
                    stats.avgPower, stats.maxPower);
        failures++;
    }
    if (stats.avgCadence != 85 || stats.calories != 50 ||
        stats.np != 180 || stats.tss != 25) {
        std::printf("FAIL: cadence/calories/NP/TSS=%u/%.1f/%.1f/%.1f\n",
                    stats.avgCadence, stats.calories, stats.np, stats.tss);
        failures++;
    }

    parser.close();
    if (!storage.remove(path)) {
        std::printf("FAIL: could not remove FIT fixture\n");
        failures++;
    }

    const char* compressedPath = "/fit_compressed_timestamp_smoke.fit";
    uint8_t data[64] = {0};
    size_t dataSize = 0;
    auto put8 = [&](uint8_t value) { data[dataSize++] = value; };
    auto put16 = [&](uint16_t value) {
        put8(static_cast<uint8_t>(value));
        put8(static_cast<uint8_t>(value >> 8));
    };
    auto put32 = [&](uint32_t value) {
        put16(static_cast<uint16_t>(value));
        put16(static_cast<uint16_t>(value >> 16));
    };
    const FitFieldDef compressedRecordFields[] = {
        {253, 4, FitBaseType::UINT32}, {5, 4, FitBaseType::UINT32},
        {6, 2, FitBaseType::UINT16}, {3, 1, FitBaseType::UINT8},
        {7, 2, FitBaseType::UINT16},
    };

    put8(0x40); put8(0); put8(0); put16(FitMesg::RECORD);
    put8(5);
    for (const FitFieldDef& field : compressedRecordFields) {
        put8(field.num); put8(field.size); put8(field.baseType);
    }
    put8(0x00);
    put32(100); put32(0); put16(10000); put8(140); put16(180);
    put8(static_cast<uint8_t>(0x80 | 5)); // local type 0, timestamp offset 5
    put32(10000); put16(12000); put8(150); put16(190);

    uint8_t fitHeader[14] = {14, 0x20, 0, 0, 0, 0, 0, 0,
                             '.', 'F', 'I', 'T', 0, 0};
    fitHeader[4] = static_cast<uint8_t>(dataSize);
    fitHeader[5] = static_cast<uint8_t>(dataSize >> 8);
    fitHeader[6] = static_cast<uint8_t>(dataSize >> 16);
    fitHeader[7] = static_cast<uint8_t>(dataSize >> 24);
    File32 compressedFile = storage.openFile(
        compressedPath, O_WRITE | O_CREAT | O_TRUNC);
    if (!compressedFile.isOpen()) {
        std::printf("FAIL: could not create compressed-timestamp fixture\n");
        failures++;
    } else {
        compressedFile.write(fitHeader, sizeof(fitHeader));
        compressedFile.write(data, dataSize);
        const uint8_t crc[2] = {0, 0};
        compressedFile.write(crc, sizeof(crc));
        compressedFile.close();

        FitLogParser compressedParser(&storage);
        if (!compressedParser.open(&storage, compressedPath)) {
            std::printf("FAIL: could not open compressed-timestamp fixture\n");
            failures++;
        } else {
            while (compressedParser.step()) {}
            const RideStats& compressedStats = compressedParser.stats();
            if (!compressedStats.valid || compressedStats.durationSec != 1 ||
                compressedStats.distanceM != 100.0f ||
                compressedStats.avgHr != 145 || compressedStats.maxHr != 150 ||
                compressedStats.avgPower != 185 ||
                compressedStats.maxPower != 190) {
                std::printf("FAIL: compressed timestamp record fields misparsed\n");
                failures++;
            }
            compressedParser.close();
        }
        if (!storage.remove(compressedPath)) {
            std::printf("FAIL: could not remove compressed-timestamp fixture\n");
            failures++;
        }
    }

    std::printf(failures == 0 ? "SMOKE PASS\n" : "SMOKE FAIL (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}

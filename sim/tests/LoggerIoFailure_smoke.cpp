// Standalone smoke test for logger write-failure propagation. Build and run
// from sim/ after `pio run`:
//   g++ -std=gnu++17 -I stubs -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\SdFat\src tests\LoggerIoFailure_smoke.cpp ..\src\Loggers\CSVLogger.cpp ..\src\Loggers\TCXLogger.cpp ..\src\Loggers\FITLogger.cpp ..\src\Loggers\FITWriter.cpp stubs\Arduino.cpp stubs\SdFat.cpp -o build\LoggerIoFailure_smoke.exe
//   build\LoggerIoFailure_smoke.exe

#include "DataModel/DataModel.hpp"
#include "Loggers/CSVLogger.hpp"
#include "Loggers/FITLogger.hpp"
#include "Loggers/TCXLogger.hpp"
#include "SdFat.h"

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

    bool rename(const char* oldPath, const char* newPath) override {
        return SdFat32().rename(oldPath, newPath);
    }

    bool listFiles(const char*, StorageEntry*, uint8_t,
                   uint8_t& outCount) override {
        outCount = 0;
        return true;
    }
};

template <typename Logger>
static bool completesHealthyLog(const char* name, Logger& logger,
                                const Trackpoint& point, const timeData& start) {
    if (!logger.startLogging(start)) {
        std::printf("FAIL: %s logger failed to start healthy log\n", name);
        return false;
    }

    const bool trackpointWritten = logger.addTrackpoint(point, start);
    const bool finalised = logger.finaliseLogging();
    if (!trackpointWritten || !finalised) {
        std::printf("FAIL: %s logger failed healthy log write/finalization\n", name);
        return false;
    }
    return true;
}

template <typename Logger>
static bool rejectsWriteFailure(const char* name, Logger& logger,
                                const Trackpoint& point, const timeData& start) {
    if (!logger.startLogging(start)) {
        std::printf("FAIL: %s logger failed to start\n", name);
        return false;
    }

    simSetFileWriteFailure(true);
    const bool trackpointWritten = logger.addTrackpoint(point, start);
    simSetFileWriteFailure(false);
    const bool finalised = logger.finaliseLogging();

    if (trackpointWritten) {
        std::printf("FAIL: %s logger reported a failed write as successful\n", name);
        return false;
    }
    if (finalised) {
        std::printf("FAIL: %s logger reported failed finalization as successful\n", name);
        return false;
    }
    return true;
}

int main() {
    simSetSdRoot("build/logger_io_failure_sd");
    simSetFileWriteFailure(false);
    SdFat32 sd;
    sd.begin();
    HostStorage storage;
    DataModel model;

    const timeData healthyStart(2026, 1, 1, 11, 0, 0);
    const timeData failedStart(2026, 1, 1, 12, 0, 0);
    Trackpoint point{};
    point.speed = 20.0f;
    point.distance = 100.0f;

    CSVLogger csv(&storage, model.bus());
    FITLogger fit(&storage);
    TCXLogger tcx(&storage, model);
    bool passed = true;
    passed &= completesHealthyLog("CSV", csv, point, healthyStart);
    passed &= completesHealthyLog("FIT", fit, point, healthyStart);
    passed &= completesHealthyLog("TCX", tcx, point, healthyStart);
    passed &= rejectsWriteFailure("CSV", csv, point, failedStart);
    passed &= rejectsWriteFailure("FIT", fit, point, failedStart);
    passed &= rejectsWriteFailure("TCX", tcx, point, failedStart);

    storage.remove("/20260101_110000.csv");
    storage.remove("/20260101_120000.csv");
    storage.remove("/activity_1767265200.fit");
    storage.remove("/activity_1767268800.fit");
    storage.remove("/20260101_110000.tcx");
    storage.remove("/20260101_120000.tcx");
    storage.remove("/lap_0_0");

    std::printf(passed ? "LOGGER IO FAILURE SMOKE PASS\n"
                       : "LOGGER IO FAILURE SMOKE FAIL\n");
    return passed ? 0 : 1;
}

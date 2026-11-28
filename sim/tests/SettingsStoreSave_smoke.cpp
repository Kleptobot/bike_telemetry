// Standalone smoke test for failure-safe settings replacement. Build and run
// from sim/ after `pio run`:
//   g++ -std=gnu++17 -I stubs -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\ArduinoJson\src tests\SettingsStoreSave_smoke.cpp ..\src\App\SettingsStore.cpp stubs\Arduino.cpp stubs\SdFat.cpp -o build\SettingsStoreSave_smoke.exe
//   build\SettingsStoreSave_smoke.exe

#include "App/SettingsStore.hpp"
#include "SdFat.h"

#include <cstdio>
#include <string>

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

static std::string readFile(HostStorage& storage, const char* path) {
    File32 file = storage.openFile(path, FILE_READ);
    if (!file) return {};

    std::string contents;
    while (file.available() > 0) {
        const int c = file.read();
        if (c < 0) break;
        contents.push_back(static_cast<char>(c));
    }
    file.close();
    return contents;
}

static bool ratesMatch(const GpsSettings& actual, const GpsSettings& expected) {
    for (uint8_t i = 0; i < GPS_NMEA_RATE_COUNT; ++i) {
        if (actual.nmeaRates[i] != expected.nmeaRates[i]) return false;
    }
    return true;
}

int main() {
    simSetSdRoot("build/settings-save-smoke");
    SdFat32 sd;
    sd.begin();

    HostStorage storage;
    const char* path = "/gpsConfig.txt";
    const char* tempPath = "/gpsConfig.txt.tmp";
    const char* backupPath = "/gpsConfig.txt.bak";
    if (storage.exists(path)) storage.remove(path);
    if (storage.exists(tempPath)) storage.remove(tempPath);
    if (storage.exists(backupPath)) storage.remove(backupPath);

    DataModel model;
    SettingsStore store;
    store.begin(&storage, &model);

    GpsSettings original;
    for (uint8_t i = 0; i < GPS_NMEA_RATE_COUNT; ++i) {
        original.nmeaRates[i] = static_cast<uint8_t>(i % 3);
    }
    if (!store.saveGpsSettings(original)) {
        std::printf("FAIL: could not write initial settings\n");
        return 1;
    }
    const std::string originalFile = readFile(storage, path);
    if (originalFile.empty()) {
        std::printf("FAIL: initial settings file is empty\n");
        return 1;
    }

    GpsSettings replacement;
    for (uint8_t i = 0; i < GPS_NMEA_RATE_COUNT; ++i) {
        replacement.nmeaRates[i] = static_cast<uint8_t>((i + 1) % 4);
    }
    simSetFileWriteFailure(true);
    const bool failedWriteReported = !store.saveGpsSettings(replacement);
    simSetFileWriteFailure(false);
    if (!failedWriteReported) {
        std::printf("FAIL: failed write was reported as successful\n");
        return 1;
    }
    if (readFile(storage, path) != originalFile) {
        std::printf("FAIL: failed write changed the previous settings file\n");
        return 1;
    }

    // A restart after the old-to-backup rename must restore the prior file.
    if (!storage.rename(path, backupPath)) {
        std::printf("FAIL: could not prepare backup recovery fixture\n");
        return 1;
    }
    DataModel recoveredModel;
    SettingsStore recoveryStore;
    recoveryStore.begin(&storage, &recoveredModel);
    recoveryStore.loadGpsSettings();
    if (!ratesMatch(recoveredModel.gpsSettings().get(), original) ||
        !storage.exists(path) || storage.exists(backupPath)) {
        std::printf("FAIL: interrupted replacement did not recover old settings\n");
        return 1;
    }

    if (!store.saveGpsSettings(replacement)) {
        std::printf("FAIL: valid replacement was not saved\n");
        return 1;
    }
    DataModel replacementModel;
    SettingsStore replacementStore;
    replacementStore.begin(&storage, &replacementModel);
    replacementStore.loadGpsSettings();
    if (!ratesMatch(replacementModel.gpsSettings().get(), replacement)) {
        std::printf("FAIL: replacement settings did not load correctly\n");
        return 1;
    }

    LayoutData layout;
    layout.rows = LAYOUT_MAX_ROWS;
    layout.cols = LAYOUT_MAX_COLS;
    for (uint8_t y = 0; y < layout.rows; ++y) {
        for (uint8_t x = 0; x < layout.cols; ++x) {
            layout.displays.push_back({
                Topic::GpsValid, x, y,
                static_cast<uint8_t>(x + 1), static_cast<uint8_t>(y + 1)
            });
        }
    }
    model.layout().update(layout);
    store.saveLayout();
    if (!storage.exists("/layout.txt") ||
        !readFile(storage, "/layout.txt").size()) {
        std::printf("FAIL: layout settings were not saved\n");
        return 1;
    }

    if (storage.exists(path)) storage.remove(path);
    if (storage.exists(tempPath)) storage.remove(tempPath);
    if (storage.exists(backupPath)) storage.remove(backupPath);
    if (storage.exists("/layout.txt")) storage.remove("/layout.txt");
    if (storage.exists("/layout.txt.tmp")) storage.remove("/layout.txt.tmp");
    if (storage.exists("/layout.txt.bak")) storage.remove("/layout.txt.bak");
    std::printf("PASS: failed writes preserve settings, recovery works, and layouts save\n");
    return 0;
}

// Standalone smoke test for idempotent paired-device loading. Build and run
// from sim/:
//   g++ -std=gnu++17 -I stubs -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\ArduinoJson\src tests\BluetoothDeviceLoad_smoke.cpp ..\src\HAL\Bluetooth\BluetoothSystem.cpp ..\src\HAL\Bluetooth\BT_Device.cpp ..\src\HAL\Bluetooth\csc.cpp ..\src\HAL\Bluetooth\cps.cpp ..\src\HAL\Bluetooth\hrm.cpp stubs\Arduino.cpp stubs\SdFat.cpp stubs\bluefruit.cpp -o build\BluetoothDeviceLoad_smoke.exe
//   build\BluetoothDeviceLoad_smoke.exe

#include "HAL/Bluetooth/BluetoothSystem.hpp"
#include "SdFat.h"

#include <cstdio>
#include <fstream>
#include <string>

extern const bool ENABLE_BLUETOOTH_DEBUG = false;

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

static bool writeDevices(const std::string& path, const char* name) {
    std::ofstream file(path, std::ios::trunc);
    file << "{\"devices\":[{\"name\":\"" << name
         << "\",\"type\":0,\"MAC\":[1,2,3,4,5,6]}]}";
    return file.good();
}

int main() {
    const std::string root = "build";
    simSetSdRoot(root.c_str());

    const std::string path = root + "\\devices.txt";
    if (!writeDevices(path, "First Name")) {
        std::printf("FAIL: could not write initial devices fixture\n");
        return 1;
    }

    HostStorage storage;
    BluetoothSystem::init(&storage);
    BluetoothSystem::loadDevices();
    if (BluetoothSystem::devices().size() != 1) {
        std::printf("FAIL: initial device list did not contain one device\n");
        return 1;
    }

    if (!writeDevices(path, "Updated Name")) {
        std::printf("FAIL: could not update devices fixture\n");
        return 1;
    }
    BluetoothSystem::loadDevices();
    BluetoothSystem::loadDevices();

    const auto& devices = BluetoothSystem::devices();
    if (devices.size() != 1 || !devices[0].saved ||
        std::string(devices[0].name) != "Updated Name") {
        std::printf("FAIL: repeated loads duplicated or failed to refresh the device\n");
        return 1;
    }

    std::remove(path.c_str());
    std::printf("PASS: repeated paired-device loads remain unique and update the row\n");
    return 0;
}

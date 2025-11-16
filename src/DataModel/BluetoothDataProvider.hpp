#pragma once
#include <vector>
#include <cstdint>
#include "HAL/BluetoothInterface.hpp"

class BluetoothDataProvider {
public:
    const std::vector<BluetoothDevice>& get() const { return _devices; }
    uint32_t version() const { return _version; }

    void update(const std::vector<BluetoothDevice>& devices) {
        _devices = devices;
        ++_version;
    }

private:
    std::vector<BluetoothDevice> _devices;
    uint32_t _version = 0;
};
#pragma once
#include <Arduino.h>


struct SDState {
    bool mounted = false;
    bool present = false;
};

class SDStateProvider {
public:
    const SDState& get() const { return _data; }
    uint32_t version() const { return _version; }

    void update(const SDState& newData) {
        _data = newData;
        ++_version;
    }

private:
    SDState _data = {false, false};
    uint32_t _version = 0;
};
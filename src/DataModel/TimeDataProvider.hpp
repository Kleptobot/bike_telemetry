#pragma once
#include <Arduino.h>
#include <RTClib.h>

class TimeDataProvider {
public:
    const DateTime& get() const { return _data; }
    uint32_t version() const { return _version; }

    void update(const DateTime& newData) {
        _data = newData;
        ++_version;
    }

private:
    DateTime _data{};
    uint32_t _version = 0;
};
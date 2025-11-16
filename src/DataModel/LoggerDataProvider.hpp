#pragma once
#include <Arduino.h>
#include <RTClib.h>

struct LoggerData {
    TimeSpan totalElapsed;
    TimeSpan lapElapsed;
};

class LoggerDataProvider {
public:
    const LoggerData& get() const { return _data; }
    uint32_t version() const { return _version; }

private:
    friend class App;
    void update(const LoggerData& newData) {
        _data = newData;
        ++_version;
    }
    LoggerData _data{};
    uint32_t _version = 0;
};
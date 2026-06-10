#pragma once
#include <Arduino.h>
#include <RTClib.h>

struct timeData {
    DateTime now;
    int UTCoffset;

    timeData(){}
    timeData(
        DateTime now_,
        int offset_)
        : now(now_)
        , UTCoffset(offset_){}
};

class TimeDataProvider {
public:
    const timeData& get() const { return _data; }
    uint32_t version() const { return _version; }

    void setUTCOffset(int UTCoffset) { 
        _data.UTCoffset = UTCoffset;
        ++_version;
    }

    void update(const timeData& newData) {
        _data = newData;
        ++_version;
    }

private:
    timeData _data{};
    uint32_t _version = 0;
};
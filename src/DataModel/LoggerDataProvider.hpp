#ifndef LOGGERDATAPROVIDER_H
#define LOGGERDATAPROVIDER_H

#include <Arduino.h>
#include "DataModel/TimeDataProvider.hpp"

struct LoggerData {
    timeDuration totalElapsed;
    timeDuration lapElapsed;
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
    LoggerData _data;
    uint32_t _version = 0;
};

#endif
#ifndef CSVLogger_H
#define CSVLogger_H

#include <Arduino.h>
#include "DataModel/DataBus.hpp"
#include "ILogger.hpp"
#include "HAL/StorageInterface.hpp"
#include "SdFat.h"

class CSVLogger : public ILogger {
public:
    CSVLogger(IStorage* storage, const DataBus& bus)
        : _storage(storage), _bus(bus) {}

    void startLogging(const timeData& currentTime) override;
    void addTrackpoint(const Trackpoint& tp, const timeData& currentTime) override;
    void newLap(const timeData& currentTime) override;
    bool finaliseLogging() override;

    const timeDuration elapsed_Total() const override { return _currentTime - _startTime; }
    const timeDuration elapsed_Lap() const override {
        if (laps.empty()) return timeDuration(0);
        return _currentTime - laps.back().startTime;
    }

private:
    IStorage* _storage;
    const DataBus& _bus;
    File32 file;
    char _filename[64];  // _startTime/_currentTime inherited from ILogger
    float _lastDistanceM = 0;

    void writeHeader();
};

#endif /* CSVLogger_H */
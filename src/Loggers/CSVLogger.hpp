#ifndef CSVLogger_H
#define CSVLogger_H

#include <Arduino.h>
#include "ILogger.hpp"
#include "HAL/StorageInterface.hpp"
#include "SdFat.h"

class CSVLogger : public ILogger {
public:
    explicit CSVLogger(IStorage* storage) : _storage(storage) {}

    void startLogging(const timeData& currentTime) override;
    void addTrackpoint(const Telemetry& tp, const timeData& currentTime) override;
    void newLap(const timeData& currentTime) override;
    bool finaliseLogging() override;

    const timeDuration elapsed_Total() const override { return _currentTime - _startTime; }
    const timeDuration elapsed_Lap() const override {
        if (laps.empty()) return timeDuration(0);
        return _currentTime - laps.back().startTime;
    }

private:
    IStorage* _storage;
    File32 file;
    char _filename[64];
    timeData _startTime;
    timeData _currentTime;

    void writeHeader();
};

#endif /* CSVLogger_H */
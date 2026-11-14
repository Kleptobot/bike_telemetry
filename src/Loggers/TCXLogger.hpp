#ifndef TCXLogger_H
#define TCXLogger_H

#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include "ILogger.hpp"
#include "HAL/StorageInterface.hpp"
#include "DataModel/DataModel.hpp"

#define points_per_chunk 1800

class TCXLogger : public ILogger {

  private:
    IStorage* _storage;
    DataModel& _model;
    File32 file;
    
    uint8_t buffer[512];  // Buffer for reading data
    size_t bytesRead = 0;
    char lap_name[32];

    // NOTE: no laps vector here. This class used to declare its own, shadowing
    // ILogger::laps -- so ILogger::pause() was shifting the BASE vector's lap
    // start times while TCXLogger read its own, and the lap timer time came out
    // wrong. The base vector is the single source of truth (and carries the
    // per-lap timer baseline that activeLap() needs).

    char _filename[32];  // _startTime/_currentTime inherited from ILogger

    int totalPoints=0;

    void writeLapHeader(uint16_t lapIndex, File32 *file);
    void resetTotals();
    void dataTransfer(File32 *from, File32 *to);
      
  public:
    explicit TCXLogger(IStorage* storage, DataModel& model) : _storage(storage), _model(model) {};

    void startLogging(const timeData& currentTime);
    void addTrackpoint(const Trackpoint& tp, const timeData& currentTime);
    void newLap(const timeData& currentTime);
    bool finaliseLogging();

    const timeDuration elapsed_Total() const {return activeTotal();};
    // Guarded for the same reason as FITLogger: laps.back() on an empty
    // vector is undefined behaviour.
    const timeDuration elapsed_Lap() const {
        return activeLap();
    };

};

#endif /* TCXLogger_H */
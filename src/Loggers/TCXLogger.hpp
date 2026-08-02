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

    std::vector<Lap> laps;

    char _filename[32];
    timeData _startTime;
    timeData _currentTime;
    TimeSpan _elapsed_Total, _elapsed_Lap;

    int totalPoints=0;

    void writeLapHeader(uint16_t lapIndex, File32 *file);
    void resetTotals();
    void dataTransfer(File32 *from, File32 *to);
      
  public:
    explicit TCXLogger(IStorage* storage, DataModel& model) : _storage(storage), _model(model) {};

    void startLogging(const timeData& currentTime);
    void addTrackpoint(const Trackpoint& tp);
    void newLap(timeData currentTime);
    bool finaliseLogging();

    const timeDuration elapsed_Total() const {return _currentTime-_startTime;};
    // Guarded for the same reason as FITLogger: laps.back() on an empty
    // vector is undefined behaviour.
    const timeDuration elapsed_Lap() const {
        if (laps.empty()) return timeDuration(0);
        return _currentTime-laps.back().startTime;
    };

};

#endif /* TCXLogger_H */
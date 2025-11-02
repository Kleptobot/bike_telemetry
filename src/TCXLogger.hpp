#ifndef TCXLogger_H
#define TCXLogger_H

#include <Arduino.h>
#include <SPI.h>
#include <vector>
#include <RTClib.h>
#include <SdFat.h>
#include "HAL/StorageInterface.hpp"
#include "DataModel.hpp"

#define points_per_chunk 1800

struct Trackpoint {
    DateTime currentTime;    // Time in ISO 8601 format
    double latitude;
    double longitude;
    double altitude;
    double heartRate;    // Optional
    double power;        // Optional
    double cadence;      // Optional
    double speed;        // Optional: Instantaneous speed in meters per second
    double distance;     // Cumulative distance in meters
};

struct Lap {
  DateTime startTime;
  uint32_t maxHRM;
  uint32_t avgHRM;
  uint32_t Calories;
  double avgCadence;
  double maxSpeed;
  double totalDistance;     // Cumulative distance in meters
  uint32_t parts;
};

class TCXLogger {

  private:
    IStorage* _storage;
    DataModel& _model;
    File32 file;
    
    uint8_t buffer[512];  // Buffer for reading data
    size_t bytesRead = 0;
    char lap_name[32];

    std::vector<Lap> laps;

    char _filename[32];
    DateTime _startTime;
    DateTime _currentTime;
    TimeSpan _elapsed_Total, _elapsed_Lap;

    double totalHeartBeats=0;
    double _BPM_last=0;
    double total_RPMS;
    double _CAD_last=0;
    int totalPoints=0;

    void updateTotals(const Trackpoint& tp);
    void writeLapHeader(int lapIndex, File32 *file);
    void resetTotals();
    void dataTransfer(File32 *from, File32 *to);
      
  public:
    TCXLogger(IStorage* storage, DataModel& model) : _storage(storage), _model(model) {};

    void startLogging(DateTime currentTime);
    void addTrackpoint(const Trackpoint& tp);
    void newLap(DateTime currentTime);
    bool finaliseLogging();

    TimeSpan elapsed_Total(){return _currentTime-_startTime;};
    TimeSpan elapsed_Lap(){return _currentTime-laps.back().startTime;};
    String elapsedString_Total()
    {
      TimeSpan ts = elapsed_Total();
      return String(ts.hours())+":"+String(ts.minutes())+":"+String(ts.seconds());
    }
    String elapsedString_Lap()
    {
      TimeSpan ts = elapsed_Lap();
      return String(ts.hours())+":"+String(ts.minutes())+":"+String(ts.seconds());
    }

};

#endif /* TCXLogger_H */
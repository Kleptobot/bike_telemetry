#ifndef TCXLogger_H
#define TCXLogger_H

#include <Arduino.h>
#include <SPI.h>
#include <vector>
#include "SdFat.h"
#include <RTClib.h>

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
    SdFat32* SD;
    
    uint8_t buffer[512];  // Buffer for reading data
    size_t bytesRead = 0;
    int _age = 34;
    int _mass = 75;

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
    void writeLapHeader(Lap lp, File32 file);
    void resetTotals();
    void dataTransfer(File32 from, File32 to);
      
  public:

    TCXLogger(){};
    TCXLogger(SdFat32* sd_ptr)
    {
      SD = sd_ptr;
    };

    void startLogging(DateTime currentTime);
    void addTrackpoint(const Trackpoint& tp);
    void newLap(DateTime currentTime);
    bool finaliseLogging();

    void setMass(int mass) {_mass = mass;};
    int getMass(){return _mass;};
    void setAge(int age) {_age = age;};
    int getAge(){return _age;};
    TimeSpan elapsed_Total(){return _currentTime-_startTime;};
    TimeSpan elapsed_Lap(){return _currentTime-_startTime;};
    String elapsedString_Total()
    {
      TimeSpan ts = elapsed_Total();
      return String(ts.hours())+":"+String(ts.minutes())+":"+String(ts.seconds());
    }

};

#endif /* TCXLogger_H */
#ifndef TCXLogger_H
#define TCXLogger_H

#include <Arduino.h>
#include <SPI.h>
#include "SdFat.h"
#include <RTClib.h>

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

class TCXLogger {

  private:
    File32 file, data_tmp;
    
    bool bFinalise_Started = false;
    bool bReading, bWriting;
    uint8_t buffer[512];  // Buffer for reading data
    size_t bytesRead = 0;

    char _filename[32];
    DateTime _startTime;

    double totalHeartRate=0;
    double maxSpeed=0;
    double totalDistance=0;     // Cumulative distance in meters
    int maxHRM=1;
    int avgHRM=1;
    int Calories=0;
    int totalPoints=0;

    void updateTotals(const Trackpoint& tp);
    void writeHeader();
    void writeLaps();
    void writeFooter();
      
  public:
    TCXLogger() : file() {};

    void startLogging(DateTime currentTime);
    void addTrackpoint(const Trackpoint& tp);
    bool finaliseLogging();

};

#endif /* TCXLogger_H */
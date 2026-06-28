#ifndef LOGGERINTERFACE_H
#define LOGGERINTERFACE_H

#include "DataModel/TimeDataProvider.hpp"

struct Trackpoint {
    timeData currentTime;    // Time in ISO 8601 format
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
  timeData startTime;
  float maxHRM;
  float totalHRM;
  float totalCadence;
  float maxSpeed;
  float totalDistance;     // Cumulative distance in meters
  uint16_t parts;
};

class ILogger {
public:
    virtual void startLogging(const timeData& currentTime) =0 ;
    virtual void addTrackpoint(const Trackpoint& tp) = 0;
    virtual void newLap(timeData currentTime) = 0;
    virtual bool finaliseLogging() = 0;
    virtual const timeDuration elapsed_Total() const = 0;
    virtual const timeDuration elapsed_Lap() const = 0;
 
    // Not part of ILogger, but kept for parity with the original TCX logger's
    // public surface in case call sites use these directly.
    const String elapsedString_Total() const {
        timeDuration ts = elapsed_Total();
        return String(ts.hours()) + ":" + String(ts.minutes()) + ":" + String(ts.seconds());
    }
    const String elapsedString_Lap() const {
        timeDuration ts = elapsed_Lap();
        return String(ts.hours()) + ":" + String(ts.minutes()) + ":" + String(ts.seconds());
    }
    
    virtual ~ILogger() = default;
};

#endif /* LOGGERINTERFACE_H */
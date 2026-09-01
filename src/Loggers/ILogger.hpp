#ifndef LOGGERINTERFACE_H
#define LOGGERINTERFACE_H

#include "DataModel/DataModel.hpp"
#include <vector>

// FIT and TCX both want speed in m/s; the telemetry pipeline works in km/h.
inline double kmhToMs(double kmh) { return kmh / 3.6; }

// Exactly what a trackpoint logger needs, and nothing else. Replaces the old
// Telemetry struct in the ILogger surface -- the raw imu/dps blocks it also
// carried were consumed by nobody. Field names match the old Telemetry ones
// the logger bodies reference, so only the signatures changed.
struct Trackpoint {
    double latitude  = 0.0;   // degrees
    double longitude = 0.0;   // degrees
    float altitude   = 0.0f;  // metres
    float speed      = 0.0f;  // km/h
    float heartrate  = 0.0f;  // bpm
    float power      = 0.0f;  // watts
    float cadence    = 0.0f;  // rpm
    float distance   = 0.0f;  // metres (cumulative at this point)
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
    virtual void addTrackpoint(const Trackpoint& tp, const timeData& currentTime) = 0;
    virtual void newLap(const timeData& currentTime) = 0;
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
protected:
    std::vector<Lap> laps; // kept public to match original's `laps.back()` usage
};

#endif /* LOGGERINTERFACE_H */
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
    float power      = 0.0f;  // watts (power meter, or est-power if no meter)
    float cadence    = 0.0f;  // rpm
    float distance   = 0.0f;  // metres (cumulative at this point)
    // Fitness channels (from FitnessFusion; ride-scoped cumulative where noted)
    float calories          = 0.0f;  // kcal accumulated this ride
    float normalizedPower   = 0.0f;  // 30 s rolling NP (power meter), watts (0 if no meter)
    float intensityFactor   = 0.0f;  // NP / FTP (0 if FTP unset)
    float tss               = 0.0f;  // accumulated training stress score
    uint8_t hrZone          = 0;     // current HR zone 1-5 (0 = no HR)
    uint8_t powerZone       = 0;     // current power zone 1-5 (0 = no power/FTP)
};

struct Lap {
  timeData startTime;
  float maxHRM;
  float totalHRM;
  float totalCadence;
  float maxSpeed;
  float totalDistance = 0;     // Cumulative distance in meters
  uint16_t parts = 0;
  // Fitness accumulators (cumulative-from-last-trackpoint for the lap)
  float totalCalories = 0.0f;        // cumulative kcal from FitnessFusion
  float totalTss = 0.0f;             // cumulative TSS from FitnessFusion
  float lastNormalizedPower = 0.0f;   // last NP value seen (watts)
  float lastIntensityFactor = 0.0f;  // last IF value seen
  timeDuration activeAtStart = timeDuration(0);
  timeDuration activeAccumAtStart = timeDuration(0);
};

class ILogger {
public:
    void pause(const timeData& currentTime) {
        _activeAccum = _activeAccum + timeDuration(currentTime - _pauseAnchor);
        _pauseAnchor = currentTime;
        _currentTime = currentTime;   // freeze the ride clock at the pause
    }
    void resume(const timeData& currentTime) { _pauseAnchor = currentTime; }

    // Timer time (pauses excluded) accumulated by the given instant. Valid
    // whether or not logging is currently paused.
    timeDuration activeAt(const timeData& t) const {
        return _activeAccum + timeDuration(t - _pauseAnchor);
    }
    // Total timer time for the ride so far.
    timeDuration activeTotal() const { return activeAt(_currentTime); }
    // Timer time of the lap in progress.
    timeDuration activeLap() const {
        if (laps.empty()) return timeDuration(0);
        return activeTotal() - laps.back().activeAtStart;
    }

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
    // Shared by all loggers: activity anchors + last trackpoint time, in
    // the base so pause()/resume() can shift them for timer-time.
    timeData _startTime;
    timeData _currentTime;
    timeData _pauseAnchor;  // wall time when logging last became active
    // Timer time banked by completed (paused) segments. _startTime is the REAL
    // ride start and is never shifted, so file timestamps stay truthful.
    timeDuration _activeAccum = timeDuration(0);
    std::vector<Lap> laps; // kept public to match original's `laps.back()` usage

    // Resets every timer anchor at the start of a ride. Loggers call this from
    // startLogging() instead of assigning the anchors themselves, so a new ride
    // can never inherit the previous ride's banked active time.
    void initTimer(const timeData& startTime) {
        _startTime = _currentTime = _pauseAnchor = startTime;
        _activeAccum = timeDuration(0);
    }
};

#endif /* LOGGERINTERFACE_H */
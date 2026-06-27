#ifndef FITLOGGER_H
#define FITLOGGER_H

#include <vector>
#include "ILogger.hpp"
#include "FitWriter.hpp"
 
// ---------------------------------------------------------------------------
// FitLogger : public ILogger
//
// FIT-format counterpart to TCXLogger -- both implement ILogger, so call
// sites elsewhere in the firmware can hold an ILogger* and not care which
// concrete logger is active.
//
// Notes on the real Lap struct (from ILogger.hpp):
//   - totalDistance / totalHRM / totalCadence are treated as PER-LAP
//     accumulators (i.e. reset at each newLap(), not cumulative-for-activity
//     like Trackpoint::distance is). totalHRM/totalCadence are running sums
//     intended for averaging (sum / parts), matching `parts` as sample count.
//   - If your TCXLogger actually treats any of these differently (e.g.
//     totalDistance as activity-cumulative), say so and I'll adjust --
//     the two loggers should agree on what these fields mean since they
//     share the one Lap struct.
// ---------------------------------------------------------------------------
 
class FITLogger : public ILogger {
public:
    // storage must outlive the FitLogger.
    explicit FITLogger(IStorage* storage) : _writer(storage) {}
 
    void startLogging(const timeData& currentTime) override;
    void addTrackpoint(const Trackpoint& tp) override;
    void newLap(timeData currentTime) override;
    bool finaliseLogging() override;
 
    const timeDuration elapsed_Total() const override { return _currentTime - _startTime; }
    const timeDuration elapsed_Lap() const override { return _currentTime - laps.back().startTime; }
 
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
 
    std::vector<Lap> laps; // kept public to match original's `laps.back()` usage
 
private:
    FitWriter _writer;
    timeData _startTime;
    timeData _currentTime;
 
    double _lastDistanceM = 0;   // most recent cumulative distance seen (from Trackpoint::distance)
    double _lapStartDistanceM = 0; // cumulative distance at the start of the current lap,
                                    // used to derive this lap's totalDistance on rollover
 
    bool _definitionsWritten = false;
    void writeDefinitionsOnce();
 
    // local message type IDs (0-15 max per FIT spec; we use a handful)
    static constexpr uint8_t LOCAL_FILE_ID  = 0;
    static constexpr uint8_t LOCAL_RECORD   = 1;
    static constexpr uint8_t LOCAL_LAP      = 2;
    static constexpr uint8_t LOCAL_SESSION  = 3;
    static constexpr uint8_t LOCAL_ACTIVITY = 4;
 
    void writeLapMessage(const Lap& lap, uint32_t startTimeFit, uint32_t endTimeFit);
    void writeSessionMessage(uint32_t endTimeFit);
    void writeActivityMessage(uint32_t endTimeFit);
};

#endif  /* FITLOGGER_H */
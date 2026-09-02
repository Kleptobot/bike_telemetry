#ifndef FTP_ESTIMATOR_H
#define FTP_ESTIMATOR_H

#include <Arduino.h>

// =============================================================================
// FtpEstimator -- estimates FTP from the best sustained power during a ride.
//
// Standard protocol: best 20-minute average power x 0.95. We maintain a
// circular buffer of 1-second power samples (1200 = 20 min) and track the
// best rolling average seen. The estimate is available any time but is most
// meaningful after a hard sustained effort.
//
// Uses uint16_t samples (watts) -- no human produces > 65,535 W, and integer
// precision is more than adequate for a 20-minute average. Buffer costs
// 1200 x 2 = 2.4 KB RAM.
// =============================================================================

class FtpEstimator {
public:
    // 20 minutes at 1 sample/sec.
    static constexpr uint16_t WINDOW_SECONDS = 1200;

    FtpEstimator();

    // Push one second's worth of power (watts). Call once per second during
    // a ride with the best available power source: power meter if live,
    // otherwise the physics estimate.
    void addSample(uint16_t watts);

    // Best 20-minute average seen this ride, in watts. 0 if fewer than
    // WINDOW_SECONDS samples have been collected.
    uint16_t best20MinAvg() const { return _bestAvg; }

    // Standard FTP estimate: best 20-min avg x 0.95. Returns 0 if no
    // qualifying window yet.
    uint16_t suggestedFtp() const;

    // True once at least one full 20-minute window has elapsed.
    bool hasQualifyingWindow() const { return _bestAvg > 0; }

    // Number of seconds collected this ride (capped at WINDOW_SECONDS).
    uint16_t sampleCount() const { return _count; }

    // Reset for a new ride. Called from App on startLogging().
    void reset();

private:
    uint16_t _buf[WINDOW_SECONDS];
    uint16_t _idx;        // next write position
    uint16_t _count;      // samples collected (caps at WINDOW_SECONDS)
    uint32_t _sum;        // running sum of samples in buffer
    uint16_t _bestAvg;    // best rolling average seen (watts)
};

#endif /* FTP_ESTIMATOR_H */
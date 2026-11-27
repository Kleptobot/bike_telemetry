#ifndef LOGSUMMARY_HPP
#define LOGSUMMARY_HPP

// Aggregate of the metrics a log parser can extract from a saved ride. All
// fields default to zero/invalid; a parser fills them as it reads the file.
struct RideStats {
    bool valid = false;        // true once enough of the file was parsed
    uint32_t durationSec = 0;  // total timer time (seconds)
    float distanceM = 0;       // total distance (metres)
    uint16_t lapCount = 0;     // number of laps
    float avgSpeedKmh = 0;
    float maxSpeedKmh = 0;
    uint16_t avgHr = 0;
    uint16_t maxHr = 0;
    uint16_t avgPower = 0;
    uint16_t maxPower = 0;
    uint16_t avgCadence = 0;
    float calories = 0;        // kcal
    float np = 0;              // normalised power (W)
    float tss = 0;             // training stress score
};

#endif /* LOGSUMMARY_HPP */

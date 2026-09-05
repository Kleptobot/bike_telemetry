#ifndef LOGSUMMARY_H
#define LOGSUMMARY_H

#include <Arduino.h>
#include "HAL/StorageInterface.hpp"

// Aggregate metrics for one saved ride, derived from a log file. Filled by a
// per-format parser; on-device summary screens render these directly.
struct RideStats {
    bool     valid       = false;
    uint32_t durationSec = 0;
    float    distanceM   = 0;
    uint16_t lapCount    = 1;
    float    maxSpeedKmh = 0;
    float    avgSpeedKmh = 0;
    float    maxHr       = 0;
    float    avgHr       = 0;
    float    maxPower    = 0;
    float    avgPower    = 0;
    float    avgCadence  = 0;
    float    calories    = 0;
    float    np          = 0;
    float    tss         = 0;
};

// Chunked CSV reader: open() a log line, then call step() repeatedly from
// the owning screen's update() so a large file is parsed a little per frame
// without a long blocking read. done() is true after EOF is consumed; the
// numbers only become meaningful once done().
class LogParser {
public:
    virtual ~LogParser() = default;
    virtual void reset() = 0;
    virtual bool open(IStorage* storage, const char* path) = 0;
    virtual void close() = 0;
    virtual bool step() = 0;
    virtual bool done() const = 0;
    virtual const RideStats& stats() const = 0;
};

class CsvLogParser : public LogParser {
public:
    CsvLogParser();
    void reset();
    // Opens and positions at the header. Returns false if open failed.
    bool open(IStorage* storage, const char* path);
    void close();
    // Reads a small chunk and parses any complete lines. Returns true while
    // data remains to be read; returns false once EOF is reached (and done()).
    bool step();
    bool done() const { return _done; }
    const RideStats& stats() const { return _stats; }

private:
    File32   _file;
    bool     _done;
    bool     _headerSkipped;
    char     _buf[256];        // current partial line accumulator
    uint16_t _lineLen;

    // Accumulators
    bool      _haveTime;
    long long _firstEpoch, _lastEpoch;
    long long _prevEpoch;
    float     _prevSpeed;      // speed at _prevEpoch (for distance integration)
    float     _sumHr, _sumPwr, _sumCad, _maxCad;
    uint32_t  _hrN, _pwrN, _cadN;
    RideStats _stats;

    void parseLine(const char* line);
};

// ---------------------------------------------------------------------------
// Chunked FIT binary reader. The firmware's FITLogger (see FITWriter.hpp and
// FITLogger.cpp) writes files with a fixed, known message set:
//   - FILE_ID, DEVICE_INFO definitions (not needed for summary)
//   - RECORD definition: timestamp(u32), position_lat(s32), position_long(s32),
//     altitude(u16,scale5,offset500), distance(u32,scale100), speed(u16,scale1000),
//     heart_rate(u8), cadence(u8), power(u16), calories(u16,scale10)
//   - LAP definition: timestamp(u32), start_time(u32), total_elapsed_time(u32,scale1000),
//     total_distance(u32,scale100), max_speed(u16,scale1000), avg_heart_rate(u8),
//     max_heart_rate(u8), avg_cadence(u8), total_calories(u32,scale10)
//   - SESSION definition: timestamp(u32), start_time(u32), total_elapsed_time(u32,scale1000),
//     total_distance(u32,scale100), sport(u8)
//   - ACTIVITY definition (finaliseLogging)
//
// Each data record carries exactly the fields in its definition, in definition
// order, packed little-endian. The parser walks the byte stream in chunks: it
// builds a per-local-type field map from each definition record, then decodes
// data records field-by-field using that map. LAP/SPEED records are consumed
// for aggregates; RECORD records are skipped (only needed for per-second detail).
// ---------------------------------------------------------------------------
class FitLogParser : public LogParser {
public:
    FitLogParser();
    void reset();
    // Opens and skips the 14-byte FIT header (+ optional 2-byte header CRC).
    // Returns false if open failed or the magic ".FIT" tag is missing, so a
    // non-FIT file is rejected cleanly rather than mis-parsed.
    bool open(IStorage* storage, const char* path);
    void close();
    // Reads a small chunk and decodes any complete records in it. Returns true
    // while data remains; false once EOF (file size bytes consumed + CRC read).
    bool step();
    bool done() const { return _done; }
    const RideStats& stats() const { return _stats; }

private:
    File32   _file;
    bool     _done;
    uint32_t _fileSize;
    uint32_t _pos;             // current read offset within data region
    RideStats _stats;

    // Per-local-type field map: {fieldNumber, sizeBytes} for the most recently
    // seen definition of that local type. Only types we emit summaries for
    // (LAP, SESSION, RECORD) are tracked.
    struct FieldMap {
        uint8_t num;
        uint8_t size;
    };
    struct LocalDef {
        bool     used;         // have we seen a definition for this local type?
        uint16_t globalNum;    // FitMesg::*  (253, 19, 18, etc.)
        FieldMap fields[32];   // max 32 fields per our definitions
        uint8_t  numFields;
    };
    LocalDef _defs[16];        // local types 0..15

    uint8_t _partialBuf[32];
    uint8_t _partialLen;

    void resetLocal();
    void clearDefs();
    void applyDefinition(uint8_t recHdr, const uint8_t* p, size_t avail);
    void applyDataRecord(uint8_t recHdr, const uint8_t* p, size_t avail);
    void handleLapRecord(const LocalDef& def, const uint8_t* p);
    void handleSessionRecord(const LocalDef& def, const uint8_t* p);
};

// ---------------------------------------------------------------------------
// Chunked TCX (XML) reader. The firmware's TCXLogger writes <Lap> elements
// with aggregates: TotalTimeSeconds, DistanceMeters, MaximumSpeed (m/s),
// Calories, AverageHeartRateBpm, MaximumHeartRateBpm, Cadence, plus
// Extensions (AvgNormalizedPower, Tss). The parser accumulates across laps:
// sums duration/distance/calories, takes max speed/HR, averages HR/cadence.
// Speed is converted m/s -> km/h for the summary.
// ---------------------------------------------------------------------------
class TcxLogParser : public LogParser {
public:
    TcxLogParser();
    void reset();
    bool open(IStorage* storage, const char* path);
    void close();
    bool step();
    bool done() const { return _done; }
    const RideStats& stats() const { return _stats; }

private:
    File32   _file;
    bool     _done;
    char     _buf[256];
    uint16_t _lineLen;

    // Accumulators
    float    _sumHr = 0;
    float    _sumCadence = 0;
    uint16_t _hrN = 0;
    uint16_t _cadN = 0;
    RideStats _stats;

    enum class HrExpect { None, Avg, Max };
    HrExpect _hrExpect = HrExpect::None;

    void processLine(const char* line);
    static float extractBetween(const char* line, const char* open, const char* close);
};

#endif /* LOGSUMMARY_H */
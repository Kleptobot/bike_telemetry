#pragma once

#include "Loggers/LogParser.hpp"
#include <stdint.h>

class CsvLogParser : public LogParser {
public:
    using LogParser::LogParser;
    bool open(IStorage* storage, const char* path) override;
    bool step() override;
    bool done() const override { return _done; }
    void close() override;

private:
    static constexpr uint8_t kMaxColumns = 80;
    static constexpr size_t kReadBufferSize = 128;
    static constexpr size_t kLineBufferSize = 1024;

    File32 _file;
    char _readBuffer[kReadBufferSize];
    size_t _readPos = 0;
    size_t _readLen = 0;
    char _lineBuffer[kLineBufferSize];
    size_t _lineLen = 0;
    int8_t _timeColumn = -1;
    int8_t _speedColumn = -1;
    int8_t _hrColumn = -1;
    int8_t _cadenceColumn = -1;
    int8_t _powerColumn = -1;
    int8_t _caloriesColumn = -1;
    int8_t _npColumn = -1;
    int8_t _tssColumn = -1;
    int8_t _distanceColumn = -1;
    uint64_t _hrTotal = 0;
    uint64_t _powerTotal = 0;
    uint64_t _cadenceTotal = 0;
    uint32_t _hrCount = 0;
    uint32_t _powerCount = 0;
    uint32_t _cadenceCount = 0;
    uint32_t _speedCount = 0;
    float _speedTotal = 0;
    float _maxPower = 0;
    float _lastDistanceM = 0;
    float _lastCalories = 0;
    float _lastNp = 0;
    float _lastTss = 0;
    long long _firstTimestamp = 0;
    long long _lastTimestamp = 0;
    uint32_t _summaryDurationSec = 0;
    uint16_t _summaryLapCount = 0;
    float _summaryDistanceM = 0;
    bool _headerSeen = false;
    bool _hasRows = false;
    bool _hasDistance = false;
    bool _hasTimestamp = false;
    bool _summarySeen = false;
    bool _lineTooLong = false;
    bool _done = false;

    void _reset();
    bool _processLine();
    bool _parseHeader(char* line);
    bool _parseSummary(char* line);
    bool _parseDataRow(char* line);
    bool _splitColumns(char* line, char* columns[], uint8_t& count) const;
    bool _parseFloat(const char* text, float& value) const;
    bool _parseUnsigned(const char* text, uint32_t& value) const;
    bool _parseTimestamp(const char* text, long long& timestamp) const;
    void _finalise();
    void _fail();
};

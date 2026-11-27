#include "Loggers/CsvLogParser.hpp"
#include "DataModel/TimeDataProvider.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void CsvLogParser::_reset() {
    _stats = RideStats{};
    _readPos = 0;
    _readLen = 0;
    _lineLen = 0;
    _timeColumn = -1;
    _speedColumn = -1;
    _hrColumn = -1;
    _cadenceColumn = -1;
    _powerColumn = -1;
    _caloriesColumn = -1;
    _npColumn = -1;
    _tssColumn = -1;
    _distanceColumn = -1;
    _hrTotal = 0;
    _powerTotal = 0;
    _cadenceTotal = 0;
    _hrCount = 0;
    _powerCount = 0;
    _cadenceCount = 0;
    _speedCount = 0;
    _speedTotal = 0;
    _maxPower = 0;
    _lastDistanceM = 0;
    _lastCalories = 0;
    _lastNp = 0;
    _lastTss = 0;
    _firstTimestamp = 0;
    _lastTimestamp = 0;
    _summaryDurationSec = 0;
    _summaryLapCount = 0;
    _summaryDistanceM = 0;
    _headerSeen = false;
    _hasRows = false;
    _hasDistance = false;
    _hasTimestamp = false;
    _summarySeen = false;
    _lineTooLong = false;
    _done = false;
}

bool CsvLogParser::open(IStorage* storage, const char* path) {
    close();
    _reset();
    if (!storage) return false;
    _file = storage->openFile(path, O_READ);
    return _file.isOpen();
}

void CsvLogParser::close() {
    if (_file.isOpen()) _file.close();
    _done = true;
}

bool CsvLogParser::_splitColumns(char* line, char* columns[],
                                        uint8_t& count) const {
    count = 0;
    if (!line || !*line) return true;
    columns[count++] = line;
    for (char* p = line; *p; ++p) {
        if (*p != ',') continue;
        if (count >= kMaxColumns) return false;
        *p = '\0';
        columns[count++] = p + 1;
    }
    return true;
}

bool CsvLogParser::_parseHeader(char* line) {
    char* columns[kMaxColumns];
    uint8_t count;
    if (!_splitColumns(line, columns, count)) return false;

    for (uint8_t i = 0; i < count; ++i) {
        if (strcmp(columns[i], "time") == 0) _timeColumn = i;
        else if (strcmp(columns[i], "estimatedspeed") == 0 ||
                 strcmp(columns[i], "speed") == 0) _speedColumn = i;
        else if (strcmp(columns[i], "heartrate") == 0) _hrColumn = i;
        else if (strcmp(columns[i], "cadence") == 0) _cadenceColumn = i;
        else if (strcmp(columns[i], "power") == 0) _powerColumn = i;
        else if (strcmp(columns[i], "calories") == 0) _caloriesColumn = i;
        else if (strcmp(columns[i], "normalizedpower") == 0) _npColumn = i;
        else if (strcmp(columns[i], "tss") == 0) _tssColumn = i;
        else if (strcmp(columns[i], "distance") == 0) _distanceColumn = i;
    }

    _headerSeen = _timeColumn >= 0 && _speedColumn >= 0 && _hrColumn >= 0 &&
                  _cadenceColumn >= 0 && _powerColumn >= 0 &&
                  _caloriesColumn >= 0 && _npColumn >= 0 && _tssColumn >= 0;
    return _headerSeen;
}

bool CsvLogParser::_parseFloat(const char* text, float& value) const {
    if (!text || !*text) return false;
    char* end = nullptr;
    value = strtof(text, &end);
    if (end == text) return false;
    while (*end == ' ' || *end == '\t') ++end;
    return *end == '\0' && value == value &&
           value <= 3.402823e+38f && value >= -3.402823e+38f;
}

bool CsvLogParser::_parseUnsigned(const char* text,
                                         uint32_t& value) const {
    if (!text || !*text || *text == '-') return false;
    char* end = nullptr;
    unsigned long parsed = strtoul(text, &end, 10);
    if (end == text) return false;
    while (*end == ' ' || *end == '\t') ++end;
    if (*end != '\0' || parsed > 0xFFFFFFFFUL) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool CsvLogParser::_parseTimestamp(const char* text,
                                          long long& timestamp) const {
    int year, month, day, hour, minute, second;
    if (!text ||
        sscanf(text, "%4d-%2d-%2dT%2d:%2d:%2d",
               &year, &month, &day, &hour, &minute, &second) != 6 ||
        month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 60) {
        return false;
    }
    timestamp = timeData(year, month, day, hour, minute, second).unixtime();
    return true;
}

bool CsvLogParser::_parseSummary(char* line) {
    char* columns[kMaxColumns];
    uint8_t count;
    if (!_splitColumns(line, columns, count) || count != 5 ||
        strcmp(columns[0], "#summary") != 0) {
        return false;
    }

    uint32_t version, duration, lapCount;
    if (!_parseUnsigned(columns[1], version) || version != 1 ||
        !_parseUnsigned(columns[2], duration) ||
        !_parseUnsigned(columns[4], lapCount) || lapCount > 0xFFFF) {
        return false;
    }
    float distance;
    if (!_parseFloat(columns[3], distance) || distance < 0) return false;

    _summaryDurationSec = duration;
    _summaryDistanceM = distance;
    _summaryLapCount = static_cast<uint16_t>(lapCount);
    _summarySeen = true;
    return true;
}

bool CsvLogParser::_parseDataRow(char* line) {
    char* columns[kMaxColumns];
    uint8_t count;
    if (!_splitColumns(line, columns, count)) return false;

    const int8_t required[] = {
        _timeColumn, _speedColumn, _hrColumn, _cadenceColumn,
        _powerColumn, _caloriesColumn, _npColumn, _tssColumn,
    };
    for (uint8_t i = 0; i < sizeof(required) / sizeof(required[0]); ++i)
        if (required[i] < 0 || count <= static_cast<uint8_t>(required[i]))
            return false;
    if (_distanceColumn >= 0 &&
        count <= static_cast<uint8_t>(_distanceColumn)) return false;

    long long timestamp;
    if (!_parseTimestamp(columns[_timeColumn], timestamp)) return false;
    if (!_hasTimestamp) {
        _firstTimestamp = timestamp;
        _hasTimestamp = true;
    }
    _lastTimestamp = timestamp;

    float speed, hr, cadence, power, calories, np, tss;
    if (!_parseFloat(columns[_speedColumn], speed) ||
        !_parseFloat(columns[_hrColumn], hr) ||
        !_parseFloat(columns[_cadenceColumn], cadence) ||
        !_parseFloat(columns[_powerColumn], power) ||
        !_parseFloat(columns[_caloriesColumn], calories) ||
        !_parseFloat(columns[_npColumn], np) ||
        !_parseFloat(columns[_tssColumn], tss)) {
        return false;
    }

    if (speed > 0) {
        _speedTotal += speed;
        ++_speedCount;
        if (speed > _stats.maxSpeedKmh) _stats.maxSpeedKmh = speed;
    }
    if (hr > 0) {
        _hrTotal += static_cast<uint32_t>(hr);
        ++_hrCount;
        if (hr > _stats.maxHr) _stats.maxHr = static_cast<uint16_t>(hr);
    }
    if (cadence > 0) {
        _cadenceTotal += static_cast<uint32_t>(cadence);
        ++_cadenceCount;
    }
    if (power > 0) {
        _powerTotal += static_cast<uint32_t>(power);
        ++_powerCount;
        if (power > _maxPower) _maxPower = power;
    }
    if (_distanceColumn >= 0) {
        float distance;
        if (!_parseFloat(columns[_distanceColumn], distance) || distance < 0)
            return false;
        _lastDistanceM = distance;
        _hasDistance = true;
    }

    _lastCalories = calories;
    _lastNp = np;
    _lastTss = tss;
    _hasRows = true;
    return true;
}

bool CsvLogParser::_processLine() {
    if (_lineLen == 0) return true;
    _lineBuffer[_lineLen] = '\0';

    if (!_headerSeen) return _parseHeader(_lineBuffer);
    if (_summarySeen) return false;
    if (strncmp(_lineBuffer, "#summary,", 9) == 0)
        return _parseSummary(_lineBuffer);
    return _parseDataRow(_lineBuffer);
}

void CsvLogParser::_finalise() {
    if (_summarySeen) {
        _stats.durationSec = _summaryDurationSec;
        _stats.distanceM = _summaryDistanceM;
        _stats.lapCount = _summaryLapCount;
    } else {
        if (_hasTimestamp && _lastTimestamp > _firstTimestamp)
            _stats.durationSec = static_cast<uint32_t>(
                _lastTimestamp - _firstTimestamp);
        if (_hasDistance) _stats.distanceM = _lastDistanceM;
    }

    if (_hrCount > 0)
        _stats.avgHr = static_cast<uint16_t>(_hrTotal / _hrCount);
    if (_powerCount > 0)
        _stats.avgPower = static_cast<uint16_t>(_powerTotal / _powerCount);
    if (_powerCount > 0)
        _stats.maxPower = static_cast<uint16_t>(_maxPower);
    if (_cadenceCount > 0)
        _stats.avgCadence = static_cast<uint16_t>(_cadenceTotal / _cadenceCount);
    _stats.calories = _lastCalories;
    _stats.np = _lastNp;
    _stats.tss = _lastTss;

    if (_stats.durationSec > 0 && _stats.distanceM > 0)
        _stats.avgSpeedKmh = _stats.distanceM * 3.6f / _stats.durationSec;
    else if (_speedCount > 0)
        _stats.avgSpeedKmh = _speedTotal / _speedCount;

    _stats.valid = _headerSeen && (_summarySeen || _hasRows);
}

void CsvLogParser::_fail() {
    _stats.valid = false;
    _done = true;
    if (_file.isOpen()) _file.close();
}

bool CsvLogParser::step() {
    if (_done) return false;

    if (_readPos >= _readLen) {
        _readPos = 0;
        int read = _file.read(_readBuffer, sizeof(_readBuffer));
        if (read < 0) {
            _fail();
            return false;
        }
        _readLen = static_cast<size_t>(read);
        if (_readLen == 0) {
            if (_lineTooLong || !_processLine()) {
                _fail();
                return false;
            }
            _finalise();
            _done = true;
            return false;
        }
    }

    while (_readPos < _readLen) {
        const char ch = _readBuffer[_readPos++];
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (_lineTooLong || !_processLine()) {
                _fail();
                return false;
            }
            _lineLen = 0;
            _lineTooLong = false;
            continue;
        }
        if (_lineLen >= sizeof(_lineBuffer) - 1) {
            _lineTooLong = true;
        } else if (!_lineTooLong) {
            _lineBuffer[_lineLen++] = ch;
        }
    }
    return true;
}

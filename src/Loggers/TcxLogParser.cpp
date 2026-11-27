#include "Loggers/TcxLogParser.hpp"
#include "DataModel/TimeDataProvider.hpp"
#include <stdio.h>
#include <string.h>

void TcxLogParser::close() {
    if (_file.isOpen()) _file.close();
    _done = true;
}

bool TcxLogParser::open(IStorage* storage, const char* path) {
    if (!storage) return false;
    close();
    _file = storage->openFile(path, O_READ);
    if (!_file) return false;

    yxml_init(&_yxml, _yxmlStack, sizeof(_yxmlStack));
    _readPos = 0;
    _readLen = 0;
    _eofReached = false;
    _done = false;
    _elemDepth = 0;
    _currentText = "";
    _totalHr = _hrCount = 0;
    _totalPower = _powerCount = 0;
    _totalCadence = _cadenceCount = 0;
    _lapHasDuration = false;
    _lapHasTrackpointTime = false;
    _lapFirstTrackpointTime = 0;
    _lapLastTrackpointTime = 0;
    return true;
}

bool TcxLogParser::step() {
    if (_done) return false;

    // Refill the read buffer when exhausted.
    if (_readPos >= _readLen) {
        _readPos = 0;
        _readLen = _file.read(_readBuf, sizeof(_readBuf));
        if (_readLen == 0) {
            _eofReached = true;
            yxml_ret_t r = yxml_eof(&_yxml);
            (void)r;
            _finalise();
            _done = true;
            return false;
        }
    }

    // Process all characters in the current buffer chunk (bounded work).
    while (_readPos < _readLen) {
        int ch = _readBuf[_readPos++];
        yxml_ret_t r = yxml_parse(&_yxml, ch);
        if (r < 0) {  // XML syntax error
            _done = true;
            return false;
        }
        _handleToken(r);
    }

    // Return true while more data may be available; false at EOF.
    return !_eofReached;
}

void TcxLogParser::_pushElem(const char* name) {
    if (_elemDepth < 12) {
        strncpy(_elemNames[_elemDepth], name, 47);
        _elemNames[_elemDepth][47] = '\0';
        _elemDepth++;
    }
}

void TcxLogParser::_popElem() {
    if (_elemDepth > 0) _elemDepth--;
}

const char* TcxLogParser::_elemParent() const {
    return _elemDepth > 1 ? _elemNames[_elemDepth - 2] : "";
}

void TcxLogParser::_handleToken(yxml_ret_t r) {
    switch (r) {
    case YXML_ELEMSTART:
        _currentText = "";
        _pushElem(_yxml.elem);
        if (strcmp(_yxml.elem, "Lap") == 0) {
            _stats.lapCount++;
            _lapHasDuration = false;
            _lapHasTrackpointTime = false;
            _lapFirstTrackpointTime = 0;
            _lapLastTrackpointTime = 0;
        }
        break;

    case YXML_CONTENT:
        _currentText.concat(_yxml.data);
        break;

    case YXML_ELEMEND:
        // YXML updates elem to the parent by ELEMEND; use our open-element
        // stack to identify the element whose content just ended.
        if (_elemDepth > 0) _processElemEnd(_elemNames[_elemDepth - 1]);
        _currentText = "";
        _popElem();
        break;

    default:
        break;
    }
}

void TcxLogParser::_processElemEnd(const char* elemName) {
    RideStats& s = _stats;
    const char* parent = _elemParent();
    float val = _currentText.toFloat();
    Serial.println("[LogParser] Elem: " + String(elemName) + ", Parent: " + String(parent) +
                   ", Text: " + _currentText + ", Val: " + String(val));
    // ---- Lap-level scalars ---------------------------------------------
    if (strcmp(elemName, "TotalTimeSeconds") == 0 && strcmp(parent, "Lap") == 0) {
        const uint32_t lapDuration = (uint32_t)val;
        s.durationSec += lapDuration;
        _lapHasDuration = lapDuration > 0;
        Serial.print("[LogParser] Lap duration: "); Serial.print(lapDuration); Serial.println(" sec");
    } else if (strcmp(elemName, "DistanceMeters") == 0 && strcmp(parent, "Lap") == 0) {
        s.distanceM = val;
    } else if (strcmp(elemName, "MaximumSpeed") == 0 && strcmp(parent, "Lap") == 0) {
        float kmh = val * 3.6f;
        if (kmh > s.maxSpeedKmh) s.maxSpeedKmh = kmh;
        Serial.print("[LogParser] Lap max speed: "); Serial.print(kmh); Serial.println(" km/h");
    } else if (strcmp(elemName, "Calories") == 0 && strcmp(parent, "Lap") == 0) {
        s.calories = val;
        Serial.print("[LogParser] Lap calories: "); Serial.print(val); Serial.println(" kcal");
    } else if (strcmp(elemName, "Value") == 0 &&
               strcmp(parent, "AverageHeartRateBpm") == 0) {
        s.avgHr = (uint16_t)val;
        Serial.print("[LogParser] Lap avg HR: "); Serial.print(s.avgHr); Serial.println(" bpm");
    } else if (strcmp(elemName, "Value") == 0 &&
               strcmp(parent, "MaximumHeartRateBpm") == 0) {
        s.maxHr = (uint16_t)val;
        Serial.print("[LogParser] Lap max HR: "); Serial.print(s.maxHr); Serial.println(" bpm");
    } else if (strcmp(elemName, "Cadence") == 0 && strcmp(parent, "Lap") == 0) {
        s.avgCadence = (uint16_t)val;
        Serial.print("[LogParser] Lap avg cadence: "); Serial.print(s.avgCadence); Serial.println(" rpm");
    } else if (strcmp(elemName, "AvgNormalizedPower") == 0 &&
               strcmp(parent, "LX") == 0) {
        s.np = val;
    } else if (strcmp(elemName, "TCXTrainingMetrics_TSS") == 0 &&
               strcmp(parent, "LX") == 0) {
        s.tss = val;

    // Some TCX writers omit TotalTimeSeconds or write it as zero. In that
    // case the Trackpoint timestamps still provide the lap's elapsed span.
    } else if (strcmp(elemName, "Time") == 0 &&
               strcmp(parent, "Trackpoint") == 0) {
        long long timestamp;
        if (_parseTimestamp(_currentText, timestamp)) {
            if (!_lapHasTrackpointTime) {
                _lapFirstTrackpointTime = timestamp;
                _lapHasTrackpointTime = true;
            }
            _lapLastTrackpointTime = timestamp;
        }
    } else if (strcmp(elemName, "Lap") == 0) {
        if (!_lapHasDuration && _lapHasTrackpointTime &&
            _lapLastTrackpointTime > _lapFirstTrackpointTime) {
            s.durationSec += (uint32_t)(_lapLastTrackpointTime -
                                        _lapFirstTrackpointTime);
        }

    // ---- Trackpoint-level samples --------------------------------------
    } else if (strcmp(elemName, "Value") == 0 &&
               strcmp(parent, "HeartRateBpm") == 0) {
        uint16_t hr = (uint16_t)val;
        _totalHr += hr;
        _hrCount++;
        if (hr > s.maxHr) s.maxHr = hr;
    } else if (strcmp(elemName, "Power") == 0 && strcmp(parent, "TPX") == 0) {
        uint16_t pwr = (uint16_t)val;
        _totalPower += pwr;
        _powerCount++;
        if (pwr > s.maxPower) s.maxPower = pwr;
    } else if (strcmp(elemName, "Cadence") == 0 && strcmp(parent, "TPX") == 0) {
        uint16_t cad = (uint16_t)val;
        _totalCadence += cad;
        _cadenceCount++;
    } else if (strcmp(elemName, "Speed") == 0 && strcmp(parent, "TPX") == 0) {
        float kmh = val * 3.6f;
        if (kmh > s.maxSpeedKmh) s.maxSpeedKmh = kmh;
    }
}

bool TcxLogParser::_parseTimestamp(const String& text,
                                         long long& timestamp) const {
    int year, month, day, hour, minute, second;
    if (sscanf(text.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
               &year, &month, &day, &hour, &minute, &second) != 6 ||
        month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 60) {
        return false;
    }

    timestamp = timeData(year, month, day, hour, minute, second).unixtime();
    return true;
}

void TcxLogParser::_finalise() {
    RideStats& s = _stats;
    if (_hrCount > 0)      s.avgHr = (uint16_t)(_totalHr / _hrCount);
    if (_powerCount > 0)   s.avgPower = (uint16_t)(_totalPower / _powerCount);
    if (_cadenceCount > 0) s.avgCadence = (uint16_t)(_totalCadence / _cadenceCount);

    if (s.durationSec > 0)
        s.avgSpeedKmh = s.distanceM * 3.6f / (float)s.durationSec;

    s.valid = (s.lapCount > 0);
}

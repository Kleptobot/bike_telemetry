#include "Loggers/FitLogParser.hpp"
#include <string.h>

void FitLogParser::_reset() {
    _stats = RideStats{};
    memset(_definitions, 0, sizeof(_definitions));
    _dataBytesRemaining = 0;
    _lastTimestamp = 0;
    _firstRecordTimestamp = 0;
    _lastRecordTimestamp = 0;
    _recordHrCount = 0;
    _recordPowerCount = 0;
    _recordCadenceCount = 0;
    _recordHrTotal = 0;
    _recordPowerTotal = 0;
    _recordCadenceTotal = 0;
    _recordMaxHr = 0;
    _recordMaxPower = 0;
    _recordDistanceM = 0;
    _recordMaxSpeedKmh = 0;
    _recordStandardCalories = 0;
    _recordCustomCalories = 0;
    _lapElapsedSec = 0;
    _lapDistanceM = 0;
    _lapAvgSpeedWeighted = 0;
    _lapAvgSpeedWeight = 0;
    _lapAvgHrWeighted = 0;
    _lapAvgHrWeight = 0;
    _lapAvgPowerWeighted = 0;
    _lapAvgPowerWeight = 0;
    _lapAvgCadenceWeighted = 0;
    _lapAvgCadenceWeight = 0;
    _lapMaxSpeedKmh = 0;
    _lapMaxHr = 0;
    _lapMaxPower = 0;
    _lapCalories = 0;
    _lapNormalizedPower = 0;
    _lapTss = 0;
    _activityDurationSec = 0;
    _session = MessageValues{};
    _hasSession = false;
    _sawLap = false;
    _sawRecord = false;
    _hasLapDistance = false;
    _hasLapElapsed = false;
    _hasLapCalories = false;
    _hasLapNormalizedPower = false;
    _hasLapTss = false;
    _hasActivityDuration = false;
    _hasRecordTimestamp = false;
    _hasRecordStandardCalories = false;
    _done = false;
}

bool FitLogParser::_readExact(File32& file, void* data, size_t size) {
    uint8_t* bytes = static_cast<uint8_t*>(data);
    size_t readTotal = 0;
    while (readTotal < size) {
        int n = file.read(bytes + readTotal, size - readTotal);
        if (n <= 0) return false;
        readTotal += static_cast<size_t>(n);
    }
    return true;
}

bool FitLogParser::_readFileExact(void* data, size_t size) {
    return _readExact(_file, data, size);
}

bool FitLogParser::_readDataByte(uint8_t& value) {
    if (_dataBytesRemaining == 0 || !_readFileExact(&value, 1)) return false;
    --_dataBytesRemaining;
    return true;
}

bool FitLogParser::_readDataExact(void* data, size_t size) {
    if (size > _dataBytesRemaining || !_readFileExact(data, size)) return false;
    _dataBytesRemaining -= static_cast<uint32_t>(size);
    return true;
}

bool FitLogParser::_skipData(size_t size) {
    uint8_t buffer[32];
    while (size > 0) {
        size_t chunk = size < sizeof(buffer) ? size : sizeof(buffer);
        if (!_readDataExact(buffer, chunk)) return false;
        size -= chunk;
    }
    return true;
}

bool FitLogParser::open(IStorage* storage, const char* path) {
    close();
    _reset();
    if (!storage) return false;

    _file = storage->openFile(path, O_READ);
    if (!_file) return false;

    uint8_t header[12];
    uint8_t headerSize;
    if (!_readFileExact(header, sizeof(header))) {
        close();
        return false;
    }
    headerSize = header[0];
    if (headerSize < sizeof(header) || headerSize > 64 ||
        memcmp(header + 8, ".FIT", 4) != 0) {
        close();
        return false;
    }

    uint8_t remainingHeader[64 - sizeof(header)];
    if (headerSize > sizeof(header) &&
        !_readFileExact(remainingHeader, headerSize - sizeof(header))) {
        close();
        return false;
    }

    _dataBytesRemaining = static_cast<uint32_t>(header[4]) |
                          (static_cast<uint32_t>(header[5]) << 8) |
                          (static_cast<uint32_t>(header[6]) << 16) |
                          (static_cast<uint32_t>(header[7]) << 24);
    return true;
}

void FitLogParser::close() {
    if (_file.isOpen()) _file.close();
    _done = true;
}

bool FitLogParser::_readDefinition(uint8_t localType,
                                          bool hasDeveloperFields) {
    uint8_t fixed[5];
    if (!_readDataExact(fixed, sizeof(fixed))) return false;

    MessageDefinition definition{};
    definition.architecture = fixed[1];
    if (definition.architecture > 1) return false;
    definition.globalNumber = definition.architecture == 0
        ? static_cast<uint16_t>(fixed[2] | (static_cast<uint16_t>(fixed[3]) << 8))
        : static_cast<uint16_t>((static_cast<uint16_t>(fixed[2]) << 8) | fixed[3]);
    definition.fieldCount = fixed[4];
    if (definition.fieldCount > kMaxFields) return false;

    for (uint8_t i = 0; i < definition.fieldCount; ++i) {
        uint8_t field[3];
        if (!_readDataExact(field, sizeof(field))) return false;
        definition.fields[i] = {field[0], field[1], field[2]};
    }

    if (hasDeveloperFields) {
        uint8_t developerCount;
        if (!_readDataByte(developerCount) ||
            developerCount > kMaxDeveloperFields) return false;
        definition.developerFieldCount = developerCount;
        for (uint8_t i = 0; i < developerCount; ++i) {
            uint8_t field[3];
            if (!_readDataExact(field, sizeof(field))) return false;
            definition.developerFieldSizes[i] = field[1];
        }
    }

    definition.valid = true;
    _definitions[localType] = definition;
    return true;
}

bool FitLogParser::_decodeField(const FieldDefinition& field,
                                       const uint8_t* bytes,
                                       uint8_t architecture,
                                       uint32_t& value) const {
    if (field.size == 0 || field.size > 4) return false;
    value = 0;
    if (architecture == 0) {
        for (uint8_t i = 0; i < field.size; ++i)
            value |= static_cast<uint32_t>(bytes[i]) << (8 * i);
    } else {
        for (uint8_t i = 0; i < field.size; ++i)
            value = (value << 8) | bytes[i];
    }
    return true;
}

bool FitLogParser::_fieldValueIsValid(const FieldDefinition& field,
                                            uint32_t value) const {
    if (field.size == 0 || field.size > 4) return false;
    const uint8_t baseType = field.baseType & 0x1F;
    const uint32_t mask = field.size == 4
        ? 0xFFFFFFFFUL
        : (static_cast<uint32_t>(1) << (field.size * 8)) - 1;

    if (baseType == 0x0A || baseType == 0x0B || baseType == 0x0C)
        return value != 0;
    if (baseType == 0x01 || baseType == 0x03 || baseType == 0x05)
        return value != (mask >> 1);
    return value != mask;
}

void FitLogParser::_setField(MessageValues& v, uint16_t globalNumber,
                                    uint8_t fieldNumber, uint32_t value) {
    if (globalNumber == kRecordMessage) {
        switch (fieldNumber) {
        case 253: v.timestamp = value; v.hasTimestamp = true; break;
        case 5: v.distanceRaw = value; v.hasDistance = true; break;
        case 6: v.maxSpeedRaw = value; v.hasMaxSpeed = true; break;
        case 3: v.avgHr = value; v.hasAvgHr = true; break;
        case 4: v.avgCadence = value; v.hasAvgCadence = true; break;
        case 7: v.avgPower = value; v.hasAvgPower = true; break;
        case 15:
            v.customRecordCalories = value;
            v.hasCustomRecordCalories = true;
            break;
        case 33: v.calories = value; v.hasCalories = true; break;
        default: break;
        }
    } else if (globalNumber == kLapMessage) {
        switch (fieldNumber) {
        case 7: v.elapsedMs = value; v.hasElapsed = true; break;
        case 8: v.timerMs = value; v.hasTimer = true; break;
        case 9: v.distanceRaw = value; v.hasDistance = true; break;
        case 13: v.avgSpeedRaw = value; v.hasAvgSpeed = true; break;
        case 14: v.maxSpeedRaw = value; v.hasMaxSpeed = true; break;
        case 15: v.avgHr = value; v.hasAvgHr = true; break;
        case 16: v.maxHr = value; v.hasMaxHr = true; break;
        case 17: v.avgCadence = value; v.hasAvgCadence = true; break;
        case 19: v.avgPower = value; v.hasAvgPower = true; break;
        case 20: v.maxPower = value; v.hasMaxPower = true; break;
        case 6:
            v.calories = value;
            v.caloriesScaledByTen = false;
            v.hasCalories = true;
            break;
        case 32:
            v.calories = value;
            v.caloriesScaledByTen = true;
            v.hasCalories = true;
            break;
        case 26: v.normalizedPower = value; v.hasNormalizedPower = true; break;
        case 27:
            v.trainingStressScore = value;
            v.hasTrainingStressScore = true;
            break;
        default: break;
        }
    } else if (globalNumber == kSessionMessage) {
        switch (fieldNumber) {
        case 7: v.elapsedMs = value; v.hasElapsed = true; break;
        case 8: v.timerMs = value; v.hasTimer = true; break;
        case 9: v.distanceRaw = value; v.hasDistance = true; break;
        case 13: v.avgSpeedRaw = value; v.hasAvgSpeed = true; break;
        case 14: v.maxSpeedRaw = value; v.hasMaxSpeed = true; break;
        case 15: v.avgHr = value; v.hasAvgHr = true; break;
        case 16: v.maxHr = value; v.hasMaxHr = true; break;
        case 17: v.avgCadence = value; v.hasAvgCadence = true; break;
        case 19: v.avgPower = value; v.hasAvgPower = true; break;
        case 20: v.maxPower = value; v.hasMaxPower = true; break;
        case 11:
            v.calories = value;
            v.caloriesScaledByTen = false;
            v.hasCalories = true;
            break;
        case 26: v.normalizedPower = value; v.hasNormalizedPower = true; break;
        case 27:
            v.trainingStressScore = value;
            v.hasTrainingStressScore = true;
            break;
        case 25: v.lapCount = value; v.hasLapCount = true; break;
        default: break;
        }
    } else if (globalNumber == kActivityMessage && fieldNumber == 0) {
        v.timerMs = value;
        v.hasTimer = true;
    }
}

bool FitLogParser::_readDataMessage(uint8_t localType,
                                           bool compressedTimestamp,
                                           uint8_t timestampOffset) {
    const MessageDefinition& definition = _definitions[localType];
    if (!definition.valid) return false;

    MessageValues values;
    for (uint8_t i = 0; i < definition.fieldCount; ++i) {
        const FieldDefinition& field = definition.fields[i];
        if (compressedTimestamp && field.number == 253) continue;

        uint8_t bytes[4] = {0, 0, 0, 0};
        const size_t decodedSize = field.size <= sizeof(bytes) ? field.size : 0;
        if (decodedSize > 0) {
            if (!_readDataExact(bytes, decodedSize)) return false;
        }
        if (field.size > decodedSize && !_skipData(field.size - decodedSize))
            return false;

        uint32_t value;
        if (decodedSize > 0 &&
            _decodeField(field, bytes, definition.architecture, value) &&
            _fieldValueIsValid(field, value)) {
            _setField(values, definition.globalNumber, field.number, value);
        }
    }

    for (uint8_t i = 0; i < definition.developerFieldCount; ++i) {
        if (!_skipData(definition.developerFieldSizes[i])) return false;
    }

    if (compressedTimestamp) {
        uint32_t timestamp = (_lastTimestamp & ~0x1FUL) | timestampOffset;
        if (_lastTimestamp != 0 && timestamp < _lastTimestamp) timestamp += 0x20;
        values.timestamp = timestamp;
        values.hasTimestamp = true;
    }
    if (values.hasTimestamp) _lastTimestamp = values.timestamp;

    _processMessage(definition.globalNumber, values);
    return true;
}

bool FitLogParser::_readMessage() {
    uint8_t header;
    if (!_readDataByte(header)) return false;

    if (header & 0x80) {
        return _readDataMessage(static_cast<uint8_t>((header >> 5) & 0x03),
                                true, static_cast<uint8_t>(header & 0x1F));
    }

    const uint8_t localType = header & 0x0F;
    if (header & 0x40)
        return _readDefinition(localType, (header & 0x20) != 0);
    return _readDataMessage(localType, false, 0);
}

void FitLogParser::_processMessage(uint16_t globalNumber,
                                          const MessageValues& v) {
    if (globalNumber == kRecordMessage) {
        _sawRecord = true;
        if (v.hasTimestamp) {
            if (!_hasRecordTimestamp) {
                _firstRecordTimestamp = v.timestamp;
                _hasRecordTimestamp = true;
            }
            _lastRecordTimestamp = v.timestamp;
        }
        if (v.hasDistance) _recordDistanceM = v.distanceRaw / 100.0f;
        if (v.hasMaxSpeed) {
            const float speedKmh = v.maxSpeedRaw * 3.6f / 1000.0f;
            if (speedKmh > _recordMaxSpeedKmh) _recordMaxSpeedKmh = speedKmh;
        }
        if (v.hasAvgHr) {
            _recordHrTotal += v.avgHr;
            ++_recordHrCount;
            if (v.avgHr > _recordMaxHr) _recordMaxHr = v.avgHr;
        }
        if (v.hasAvgPower) {
            _recordPowerTotal += v.avgPower;
            ++_recordPowerCount;
            if (v.avgPower > _recordMaxPower)
                _recordMaxPower = v.avgPower;
        }
        if (v.hasAvgCadence) {
            _recordCadenceTotal += v.avgCadence;
            ++_recordCadenceCount;
        }
        if (v.hasCalories) {
            _recordStandardCalories = static_cast<float>(v.calories);
            _hasRecordStandardCalories = true;
        }
        if (v.hasCustomRecordCalories && v.customRecordCalories >= 100)
            _recordCustomCalories = v.customRecordCalories / 10.0f;
        return;
    }

    if (globalNumber == kLapMessage) {
        _sawLap = true;
        ++_stats.lapCount;
        const float durationSec = v.hasTimer ? v.timerMs / 1000.0f
                                  : v.hasElapsed ? v.elapsedMs / 1000.0f : 0.0f;
        if (v.hasTimer || v.hasElapsed) {
            _lapElapsedSec += durationSec;
            _hasLapElapsed = true;
        }
        if (v.hasDistance) {
            _lapDistanceM += v.distanceRaw / 100.0f;
            _hasLapDistance = true;
        }
        const float weight = durationSec > 0 ? durationSec : 1.0f;
        if (v.hasAvgSpeed) {
            _lapAvgSpeedWeighted += v.avgSpeedRaw * 3.6f / 1000.0f * weight;
            _lapAvgSpeedWeight += weight;
        }
        if (v.hasAvgHr) {
            _lapAvgHrWeighted += v.avgHr * weight;
            _lapAvgHrWeight += weight;
        }
        if (v.hasAvgPower) {
            _lapAvgPowerWeighted += v.avgPower * weight;
            _lapAvgPowerWeight += weight;
        }
        if (v.hasAvgCadence) {
            _lapAvgCadenceWeighted += v.avgCadence * weight;
            _lapAvgCadenceWeight += weight;
        }
        if (v.hasMaxSpeed) {
            const float speedKmh = v.maxSpeedRaw * 3.6f / 1000.0f;
            if (speedKmh > _lapMaxSpeedKmh) _lapMaxSpeedKmh = speedKmh;
        }
        if (v.hasMaxHr && v.maxHr > _lapMaxHr) _lapMaxHr = v.maxHr;
        if (v.hasMaxPower && v.maxPower > _lapMaxPower)
            _lapMaxPower = v.maxPower;
        if (v.hasCalories) {
            _lapCalories += v.caloriesScaledByTen
                ? v.calories / 10.0f : static_cast<float>(v.calories);
            _hasLapCalories = true;
        }
        if (v.hasNormalizedPower) {
            _lapNormalizedPower = v.normalizedPower;
            _hasLapNormalizedPower = true;
        }
        if (v.hasTrainingStressScore) {
            _lapTss = v.trainingStressScore;
            _hasLapTss = true;
        }
        return;
    }

    if (globalNumber == kSessionMessage) {
        _session = v;
        _hasSession = true;
        return;
    }

    if (globalNumber == kActivityMessage && v.hasTimer) {
        _activityDurationSec = v.timerMs / 1000.0f;
        _hasActivityDuration = true;
    }
}

void FitLogParser::_finalise() {
    RideStats& s = _stats;
    const bool sessionHasDuration = _hasSession &&
                                    (_session.hasTimer || _session.hasElapsed);
    const float sessionDurationSec = !_hasSession ? 0.0f
        : _session.hasTimer ? _session.timerMs / 1000.0f
        : _session.elapsedMs / 1000.0f;

    if (sessionHasDuration) s.durationSec = static_cast<uint32_t>(sessionDurationSec);
    else if (_hasLapElapsed) s.durationSec = static_cast<uint32_t>(_lapElapsedSec);
    else if (_hasActivityDuration)
        s.durationSec = static_cast<uint32_t>(_activityDurationSec);
    else if (_hasRecordTimestamp &&
             _lastRecordTimestamp > _firstRecordTimestamp)
        s.durationSec = _lastRecordTimestamp - _firstRecordTimestamp;

    if (_hasSession && _session.hasDistance)
        s.distanceM = _session.distanceRaw / 100.0f;
    else if (_hasLapDistance)
        s.distanceM = _lapDistanceM;
    else
        s.distanceM = _recordDistanceM;

    if (_stats.lapCount == 0 && _hasSession && _session.hasLapCount)
        s.lapCount = static_cast<uint16_t>(_session.lapCount);

    if (_hasSession && _session.hasMaxSpeed)
        s.maxSpeedKmh = _session.maxSpeedRaw * 3.6f / 1000.0f;
    else if (_lapMaxSpeedKmh > 0)
        s.maxSpeedKmh = _lapMaxSpeedKmh;
    else
        s.maxSpeedKmh = _recordMaxSpeedKmh;

    if (_hasSession && _session.hasAvgHr)
        s.avgHr = static_cast<uint16_t>(_session.avgHr);
    else if (_lapAvgHrWeight > 0)
        s.avgHr = static_cast<uint16_t>(_lapAvgHrWeighted / _lapAvgHrWeight);
    else if (_recordHrCount > 0)
        s.avgHr = static_cast<uint16_t>(_recordHrTotal / _recordHrCount);
    if (_hasSession && _session.hasMaxHr)
        s.maxHr = static_cast<uint16_t>(_session.maxHr);
    else if (_lapMaxHr > 0)
        s.maxHr = static_cast<uint16_t>(_lapMaxHr);
    else if (_recordHrCount > 0)
        s.maxHr = static_cast<uint16_t>(_recordMaxHr);

    if (_hasSession && _session.hasAvgPower)
        s.avgPower = static_cast<uint16_t>(_session.avgPower);
    else if (_lapAvgPowerWeight > 0)
        s.avgPower = static_cast<uint16_t>(_lapAvgPowerWeighted /
                                           _lapAvgPowerWeight);
    else if (_recordPowerCount > 0)
        s.avgPower = static_cast<uint16_t>(_recordPowerTotal / _recordPowerCount);
    if (_hasSession && _session.hasMaxPower)
        s.maxPower = static_cast<uint16_t>(_session.maxPower);
    else if (_lapMaxPower > 0)
        s.maxPower = static_cast<uint16_t>(_lapMaxPower);
    else if (_recordPowerCount > 0)
        s.maxPower = static_cast<uint16_t>(_recordMaxPower);

    if (_hasSession && _session.hasAvgCadence)
        s.avgCadence = static_cast<uint16_t>(_session.avgCadence);
    else if (_lapAvgCadenceWeight > 0)
        s.avgCadence = static_cast<uint16_t>(_lapAvgCadenceWeighted /
                                             _lapAvgCadenceWeight);
    else if (_recordCadenceCount > 0)
        s.avgCadence = static_cast<uint16_t>(_recordCadenceTotal /
                                             _recordCadenceCount);

    if (_hasSession && _session.hasCalories)
        s.calories = static_cast<float>(_session.calories);
    else if (_hasLapCalories)
        s.calories = _lapCalories;
    else if (_hasRecordStandardCalories)
        s.calories = _recordStandardCalories;
    else if (_recordCustomCalories > 0)
        s.calories = _recordCustomCalories;
    if (_hasSession && _session.hasNormalizedPower)
        s.np = static_cast<float>(_session.normalizedPower);
    else if (_hasLapNormalizedPower)
        s.np = _lapNormalizedPower;
    if (_hasSession && _session.hasTrainingStressScore)
        s.tss = static_cast<float>(_session.trainingStressScore);
    else if (_hasLapTss)
        s.tss = _lapTss;

    if (_hasSession && _session.hasAvgSpeed)
        s.avgSpeedKmh = _session.avgSpeedRaw * 3.6f / 1000.0f;
    else if (s.durationSec > 0 && s.distanceM > 0)
        s.avgSpeedKmh = s.distanceM * 3.6f / static_cast<float>(s.durationSec);
    else if (_lapAvgSpeedWeight > 0)
        s.avgSpeedKmh = _lapAvgSpeedWeighted / _lapAvgSpeedWeight;

    s.valid = _hasSession || _sawLap || _sawRecord;
}

void FitLogParser::_fail() {
    _done = true;
    _stats.valid = false;
    if (_file.isOpen()) _file.close();
}

bool FitLogParser::step() {
    if (_done) return false;
    if (_dataBytesRemaining == 0) {
        _finalise();
        _done = true;
        return false;
    }
    if (!_readMessage()) {
        _fail();
        return false;
    }
    if (_dataBytesRemaining == 0) {
        _finalise();
        _done = true;
        return false;
    }
    return true;
}

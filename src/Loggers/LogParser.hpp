#ifndef LOGPARSER_HPP
#define LOGPARSER_HPP

#pragma once
#pragma once

#include "HAL/StorageInterface.hpp"
#include "Loggers/LogSummary.hpp"
#include "DataModel/TimeDataProvider.hpp"
#include "yxml.h"
#include <stdio.h>
#include <stdlib.h>

// Abstract incremental log parser. A parser is opened on a saved ride file,
// then step() is called repeatedly (bounded work per call) until done(). The
// accumulated stats are available via stats().
//
// Concrete parsers decode saved ride files incrementally so large logs do not
// need to be buffered in RAM.
class LogParser {
public:
    explicit LogParser(IStorage* storage = nullptr) : _storage(storage) {}
    virtual ~LogParser() = default;

    // Open `path` for reading on `storage`. Returns true on success.
    virtual bool open(IStorage* storage, const char* path) = 0;

    // Do a bounded increment of parsing. Returns true while still parsing,
    // false when complete (or on error).
    virtual bool step() = 0;

    // True once parsing has finished.
    virtual bool done() const = 0;

    const RideStats& stats() const { return _stats; }

    // Close the underlying file handle.
    virtual void close() {}

protected:
    IStorage* _storage;
    RideStats _stats;
};

class FitLogParser : public LogParser {
public:
    using LogParser::LogParser;
    bool open(IStorage* storage, const char* path) override;
    bool step() override;
    bool done() const override { return _done; }
    void close() override;

private:
    static constexpr uint8_t kLocalTypeCount = 16;
    static constexpr uint8_t kMaxFields = 32;
    static constexpr uint8_t kMaxDeveloperFields = 16;
    static constexpr uint16_t kRecordMessage = 20;
    static constexpr uint16_t kLapMessage = 19;
    static constexpr uint16_t kSessionMessage = 18;
    static constexpr uint16_t kActivityMessage = 34;

    struct FieldDefinition {
        uint8_t number;
        uint8_t size;
        uint8_t baseType;
    };

    struct MessageDefinition {
        bool valid;
        uint8_t architecture;
        uint16_t globalNumber;
        uint8_t fieldCount;
        FieldDefinition fields[kMaxFields];
        uint8_t developerFieldCount;
        uint8_t developerFieldSizes[kMaxDeveloperFields];
    };

    struct MessageValues {
        uint32_t timestamp = 0;
        uint32_t elapsedMs = 0;
        uint32_t timerMs = 0;
        uint32_t distanceRaw = 0;
        uint32_t avgSpeedRaw = 0;
        uint32_t maxSpeedRaw = 0;
        uint32_t avgHr = 0;
        uint32_t maxHr = 0;
        uint32_t avgPower = 0;
        uint32_t maxPower = 0;
        uint32_t avgCadence = 0;
        uint32_t calories = 0;
        bool caloriesScaledByTen = false;
        uint32_t customRecordCalories = 0;
        uint32_t normalizedPower = 0;
        uint32_t trainingStressScore = 0;
        uint32_t lapCount = 0;
        bool hasTimestamp = false;
        bool hasElapsed = false;
        bool hasTimer = false;
        bool hasDistance = false;
        bool hasAvgSpeed = false;
        bool hasMaxSpeed = false;
        bool hasAvgHr = false;
        bool hasMaxHr = false;
        bool hasAvgPower = false;
        bool hasMaxPower = false;
        bool hasAvgCadence = false;
        bool hasCalories = false;
        bool hasCustomRecordCalories = false;
        bool hasNormalizedPower = false;
        bool hasTrainingStressScore = false;
        bool hasLapCount = false;
    };

    File32 _file;
    MessageDefinition _definitions[kLocalTypeCount]{};
    uint32_t _dataBytesRemaining = 0;
    uint32_t _lastTimestamp = 0;
    uint32_t _firstRecordTimestamp = 0;
    uint32_t _lastRecordTimestamp = 0;
    uint32_t _recordHrCount = 0;
    uint32_t _recordPowerCount = 0;
    uint32_t _recordCadenceCount = 0;
    uint64_t _recordHrTotal = 0;
    uint64_t _recordPowerTotal = 0;
    uint64_t _recordCadenceTotal = 0;
    float _recordMaxHr = 0;
    float _recordMaxPower = 0;
    float _recordDistanceM = 0;
    float _recordMaxSpeedKmh = 0;
    float _recordStandardCalories = 0;
    float _recordCustomCalories = 0;
    float _lapElapsedSec = 0;
    float _lapDistanceM = 0;
    float _lapAvgSpeedWeighted = 0;
    float _lapAvgSpeedWeight = 0;
    float _lapAvgHrWeighted = 0;
    float _lapAvgHrWeight = 0;
    float _lapAvgPowerWeighted = 0;
    float _lapAvgPowerWeight = 0;
    float _lapAvgCadenceWeighted = 0;
    float _lapAvgCadenceWeight = 0;
    float _lapMaxSpeedKmh = 0;
    float _lapMaxHr = 0;
    float _lapMaxPower = 0;
    float _lapCalories = 0;
    float _lapNormalizedPower = 0;
    float _lapTss = 0;
    float _activityDurationSec = 0;
    MessageValues _session;
    bool _hasSession = false;
    bool _sawLap = false;
    bool _sawRecord = false;
    bool _hasLapDistance = false;
    bool _hasLapElapsed = false;
    bool _hasLapCalories = false;
    bool _hasLapNormalizedPower = false;
    bool _hasLapTss = false;
    bool _hasActivityDuration = false;
    bool _hasRecordTimestamp = false;
    bool _hasRecordStandardCalories = false;
    bool _done = false;

    void _reset();
    bool _readExact(File32& file, void* data, size_t size);
    bool _readFileExact(void* data, size_t size);
    bool _readDataByte(uint8_t& value);
    bool _readDataExact(void* data, size_t size);
    bool _skipData(size_t size);
    bool _readDefinition(uint8_t localType, bool hasDeveloperFields);
    bool _readDataMessage(uint8_t localType, bool compressedTimestamp,
                          uint8_t timestampOffset);
    bool _readMessage();
    bool _decodeField(const FieldDefinition& field, const uint8_t* bytes,
                      uint8_t architecture, uint32_t& value) const;
    bool _fieldValueIsValid(const FieldDefinition& field,
                            uint32_t value) const;
    void _setField(MessageValues& values, uint16_t globalNumber,
                   uint8_t fieldNumber, uint32_t value);
    void _processMessage(uint16_t globalNumber, const MessageValues& values);
    void _finalise();
    void _fail();
};

// ---- FitLogParser implementation -----------------------------------------

inline void FitLogParser::_reset() {
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

inline bool FitLogParser::_readExact(File32& file, void* data, size_t size) {
    uint8_t* bytes = static_cast<uint8_t*>(data);
    size_t readTotal = 0;
    while (readTotal < size) {
        int n = file.read(bytes + readTotal, size - readTotal);
        if (n <= 0) return false;
        readTotal += static_cast<size_t>(n);
    }
    return true;
}

inline bool FitLogParser::_readFileExact(void* data, size_t size) {
    return _readExact(_file, data, size);
}

inline bool FitLogParser::_readDataByte(uint8_t& value) {
    if (_dataBytesRemaining == 0 || !_readFileExact(&value, 1)) return false;
    --_dataBytesRemaining;
    return true;
}

inline bool FitLogParser::_readDataExact(void* data, size_t size) {
    if (size > _dataBytesRemaining || !_readFileExact(data, size)) return false;
    _dataBytesRemaining -= static_cast<uint32_t>(size);
    return true;
}

inline bool FitLogParser::_skipData(size_t size) {
    uint8_t buffer[32];
    while (size > 0) {
        size_t chunk = size < sizeof(buffer) ? size : sizeof(buffer);
        if (!_readDataExact(buffer, chunk)) return false;
        size -= chunk;
    }
    return true;
}

inline bool FitLogParser::open(IStorage* storage, const char* path) {
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

inline void FitLogParser::close() {
    if (_file.isOpen()) _file.close();
    _done = true;
}

inline bool FitLogParser::_readDefinition(uint8_t localType,
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

inline bool FitLogParser::_decodeField(const FieldDefinition& field,
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

inline bool FitLogParser::_fieldValueIsValid(const FieldDefinition& field,
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

inline void FitLogParser::_setField(MessageValues& v, uint16_t globalNumber,
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

inline bool FitLogParser::_readDataMessage(uint8_t localType,
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

inline bool FitLogParser::_readMessage() {
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

inline void FitLogParser::_processMessage(uint16_t globalNumber,
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

inline void FitLogParser::_finalise() {
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

inline void FitLogParser::_fail() {
    _done = true;
    _stats.valid = false;
    if (_file.isOpen()) _file.close();
}

inline bool FitLogParser::step() {
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

class TcxLogParser : public LogParser {
public:
    using LogParser::LogParser;
    bool open(IStorage* storage, const char* path) override;
    bool step() override;
    bool done() const override { return _done; }
    void close() override {
        if (_file.isOpen()) _file.close();
        _done = true;
    }

private:
    // LibYxml streaming parser: yxml_parse() consumes one char at a time.
    // No whole-file buffer is allocated; only a fixed read chunk is used.
    yxml_t _yxml;
    unsigned char _yxmlStack[512];  // yxml element/attribute name stack

    // File streaming
    File32 _file;
    char _readBuf[256];
    size_t _readPos = 0;
    size_t _readLen = 0;
    bool _eofReached = false;

    bool _done = false;

    // Element name stack for context tracking (TCX nesting is shallow;
    // 12 levels is more than enough for TrainingCenterDatabase→Lap→Trackpoint).
    char _elemNames[12][48];
    int8_t _elemDepth = 0;

    // Accumulated character content for the element currently being parsed.
    String _currentText;

    // Running totals for trackpoint-level samples (averages computed at end).
    uint32_t _totalHr = 0, _hrCount = 0;
    uint32_t _totalPower = 0, _powerCount = 0;
    uint32_t _totalCadence = 0, _cadenceCount = 0;

    bool _lapHasDuration = false;
    bool _lapHasTrackpointTime = false;
    long long _lapFirstTrackpointTime = 0;
    long long _lapLastTrackpointTime = 0;

    void _pushElem(const char* name);
    void _popElem();
    const char* _elemParent() const;
    void _handleToken(yxml_ret_t r);
    void _processElemEnd(const char* elemName);
    void _finalise();
    bool _parseTimestamp(const String& text, long long& timestamp) const;
};

// ---- TcxLogParser implementations ----------------------------------------

inline bool TcxLogParser::open(IStorage* storage, const char* path) {
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

inline bool TcxLogParser::step() {
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

inline void TcxLogParser::_pushElem(const char* name) {
    if (_elemDepth < 12) {
        strncpy(_elemNames[_elemDepth], name, 47);
        _elemNames[_elemDepth][47] = '\0';
        _elemDepth++;
    }
}

inline void TcxLogParser::_popElem() {
    if (_elemDepth > 0) _elemDepth--;
}

inline const char* TcxLogParser::_elemParent() const {
    return _elemDepth > 1 ? _elemNames[_elemDepth - 2] : "";
}

inline void TcxLogParser::_handleToken(yxml_ret_t r) {
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

inline void TcxLogParser::_processElemEnd(const char* elemName) {
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

inline bool TcxLogParser::_parseTimestamp(const String& text,
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

inline void TcxLogParser::_finalise() {
    RideStats& s = _stats;
    if (_hrCount > 0)      s.avgHr = (uint16_t)(_totalHr / _hrCount);
    if (_powerCount > 0)   s.avgPower = (uint16_t)(_totalPower / _powerCount);
    if (_cadenceCount > 0) s.avgCadence = (uint16_t)(_totalCadence / _cadenceCount);

    if (s.durationSec > 0)
        s.avgSpeedKmh = s.distanceM * 3.6f / (float)s.durationSec;

    s.valid = (s.lapCount > 0);
}

class CsvLogParser : public LogParser {
public:
    using LogParser::LogParser;
    bool open(IStorage* storage, const char* path) override;
    bool step() override;
    bool done() const override { return _done; }
    void close() override;

private:
    static constexpr uint8_t kMaxColumns = 24;
    static constexpr size_t kReadBufferSize = 128;
    static constexpr size_t kLineBufferSize = 256;

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

// ---- CsvLogParser implementation -----------------------------------------

inline void CsvLogParser::_reset() {
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

inline bool CsvLogParser::open(IStorage* storage, const char* path) {
    close();
    _reset();
    if (!storage) return false;
    _file = storage->openFile(path, O_READ);
    return _file.isOpen();
}

inline void CsvLogParser::close() {
    if (_file.isOpen()) _file.close();
    _done = true;
}

inline bool CsvLogParser::_splitColumns(char* line, char* columns[],
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

inline bool CsvLogParser::_parseHeader(char* line) {
    char* columns[kMaxColumns];
    uint8_t count;
    if (!_splitColumns(line, columns, count)) return false;

    for (uint8_t i = 0; i < count; ++i) {
        if (strcmp(columns[i], "time") == 0) _timeColumn = i;
        else if (strcmp(columns[i], "speed") == 0) _speedColumn = i;
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

inline bool CsvLogParser::_parseFloat(const char* text, float& value) const {
    if (!text || !*text) return false;
    char* end = nullptr;
    value = strtof(text, &end);
    if (end == text) return false;
    while (*end == ' ' || *end == '\t') ++end;
    return *end == '\0' && value == value &&
           value <= 3.402823e+38f && value >= -3.402823e+38f;
}

inline bool CsvLogParser::_parseUnsigned(const char* text,
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

inline bool CsvLogParser::_parseTimestamp(const char* text,
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

inline bool CsvLogParser::_parseSummary(char* line) {
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

inline bool CsvLogParser::_parseDataRow(char* line) {
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

inline bool CsvLogParser::_processLine() {
    if (_lineLen == 0) return true;
    _lineBuffer[_lineLen] = '\0';

    if (!_headerSeen) return _parseHeader(_lineBuffer);
    if (_summarySeen) return false;
    if (strncmp(_lineBuffer, "#summary,", 9) == 0)
        return _parseSummary(_lineBuffer);
    return _parseDataRow(_lineBuffer);
}

inline void CsvLogParser::_finalise() {
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

inline void CsvLogParser::_fail() {
    _stats.valid = false;
    _done = true;
    if (_file.isOpen()) _file.close();
}

inline bool CsvLogParser::step() {
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

#endif /* LOGPARSER_HPP */

#pragma once

#include "Loggers/LogParser.hpp"
#include <stdint.h>

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

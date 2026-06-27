#include "FITLogger.hpp"

// ---------------------------------------------------------------------------
// All field numbers/base types/sizes below come from FIT Profile.xlsx.
// If you add a field not listed here, look up its real number+type --
// don't guess, or readers will misparse the file.
// ---------------------------------------------------------------------------

void FITLogger::writeDefinitionsOnce() {
    if (_definitionsWritten) return;

    // FILE_ID: type(u8,enum=0), manufacturer(u16), product(u16),
    //          serial_number(u32), time_created(u32)
    {
        FitFieldDef f[] = {
            {0, 1, FitBaseType::ENUM},   // type: 4 = activity
            {1, 2, FitBaseType::UINT16}, // manufacturer: 255 = development
            {2, 2, FitBaseType::UINT16}, // product
            {3, 4, FitBaseType::UINT32}, // serial_number
            {4, 4, FitBaseType::UINT32}, // time_created
        };
        _writer.writeDefinition(LOCAL_FILE_ID, FitMesg::FILE_ID, f, 5);
    }

    // RECORD: timestamp(u32), position_lat(s32), position_long(s32),
    //         altitude(u16, scale 5, offset 500), distance(u32, scale 100),
    //         speed(u16, scale 1000), heart_rate(u8), cadence(u8), power(u16)
    {
        FitFieldDef f[] = {
            {253, 4, FitBaseType::UINT32}, // timestamp
            {0,   4, FitBaseType::SINT32}, // position_lat (semicircles)
            {1,   4, FitBaseType::SINT32}, // position_long (semicircles)
            {2,   2, FitBaseType::UINT16}, // altitude, scale 5, offset 500 (m)
            {5,   4, FitBaseType::UINT32}, // distance, scale 100 (m), cumulative
            {6,   2, FitBaseType::UINT16}, // speed, scale 1000 (m/s)
            {3,   1, FitBaseType::UINT8},  // heart_rate (bpm, 0xFF = invalid)
            {4,   1, FitBaseType::UINT8},  // cadence (rpm, 0xFF = invalid)
            {7,   2, FitBaseType::UINT16}, // power (watts, 0xFFFF = invalid)
        };
        _writer.writeDefinition(LOCAL_RECORD, FitMesg::RECORD, f, 9);
    }

    // LAP: timestamp(u32), start_time(u32), total_elapsed_time(u32, scale 1000),
    //      total_distance(u32, scale 100), max_speed(u16, scale 1000),
    //      avg_heart_rate(u8), max_heart_rate(u8), avg_cadence(u8)
    {
        FitFieldDef f[] = {
            {253, 4, FitBaseType::UINT32}, // timestamp (= end of lap)
            {2,   4, FitBaseType::UINT32}, // start_time
            {7,   4, FitBaseType::UINT32}, // total_elapsed_time, scale 1000 (s)
            {9,   4, FitBaseType::UINT32}, // total_distance, scale 100 (m)
            {13,  2, FitBaseType::UINT16}, // max_speed, scale 1000 (m/s)
            {15,  1, FitBaseType::UINT8},  // avg_heart_rate (bpm)
            {16,  1, FitBaseType::UINT8},  // max_heart_rate (bpm)
            {17,  1, FitBaseType::UINT8},  // avg_cadence (rpm)
        };
        _writer.writeDefinition(LOCAL_LAP, FitMesg::LAP, f, 8);
    }

    // SESSION: timestamp(u32), start_time(u32), total_elapsed_time(u32,scale1000),
    //          total_distance(u32,scale100), sport(u8,enum)
    {
        FitFieldDef f[] = {
            {253, 4, FitBaseType::UINT32},
            {2,   4, FitBaseType::UINT32},
            {7,   4, FitBaseType::UINT32},
            {9,   4, FitBaseType::UINT32},
            {5,   1, FitBaseType::ENUM},   // sport: 2 = cycling
        };
        _writer.writeDefinition(LOCAL_SESSION, FitMesg::SESSION, f, 5);
    }

    // ACTIVITY: timestamp(u32), total_timer_time(u32,scale1000),
    //           num_sessions(u16), type(u8,enum), event(u8,enum), event_type(u8,enum)
    {
        FitFieldDef f[] = {
            {253, 4, FitBaseType::UINT32},
            {0,   4, FitBaseType::UINT32},
            {1,   2, FitBaseType::UINT16},
            {2,   1, FitBaseType::ENUM},
            {3,   1, FitBaseType::ENUM},
            {4,   1, FitBaseType::ENUM},
        };
        _writer.writeDefinition(LOCAL_ACTIVITY, FitMesg::ACTIVITY, f, 6);
    }

    _definitionsWritten = true;
}

void FITLogger::startLogging(const timeData& currentTime) {
    _startTime = currentTime;
    _currentTime = currentTime;
    _lastDistanceM = 0;
    _lapStartDistanceM = 0;
    laps.clear();

    // TODO: match your existing filename scheme (date/time-based, etc.)
    String path = "/activity_" + String(static_cast<unsigned long>(currentTime.unixtime())) + ".fit";
    _writer.open(path);

    writeDefinitionsOnce();

    uint32_t nowFit = toFitTimestamp(currentTime.unixtime());

    _writer.beginDataMessage(LOCAL_FILE_ID);
    _writer.writeU8(4);            // type = activity
    _writer.writeU16(255);         // manufacturer = development
    _writer.writeU16(0);           // product (your own ID once registered)
    _writer.writeU32(0);           // serial number -- consider using chip UID
    _writer.writeU32(nowFit);      // time_created

    newLap(currentTime); // implicitly start lap 0, matching typical TCX flow
}

void FITLogger::addTrackpoint(const Trackpoint& tp) {
    _currentTime = tp.currentTime;
    uint32_t ts = toFitTimestamp(tp.currentTime.unixtime());

    // altitude field is scale 5, offset 500 per FIT profile: stored = (m + 500) * 5
    uint16_t altitudeEncoded = static_cast<uint16_t>((tp.altitude + 500.0) * 5.0);

    _writer.beginDataMessage(LOCAL_RECORD);
    _writer.writeU32(ts);
    _writer.writeS32(toSemicircles(tp.latitude));
    _writer.writeS32(toSemicircles(tp.longitude));
    _writer.writeU16(altitudeEncoded);
    _writer.writeU32(static_cast<uint32_t>(tp.distance * 100.0));      // cumulative, scale 100
    _writer.writeU16(static_cast<uint16_t>(tp.speed * 1000.0));        // scale 1000
    _writer.writeU8(tp.heartRate > 0 ? static_cast<uint8_t>(tp.heartRate) : 0xFF);
    _writer.writeU8(tp.cadence > 0 ? static_cast<uint8_t>(tp.cadence) : 0xFF);
    _writer.writeU16(tp.power > 0 ? static_cast<uint16_t>(tp.power) : 0xFFFF);

    _lastDistanceM = tp.distance;

    Lap& lap = laps.back();
    lap.parts++;
    if (tp.speed > lap.maxSpeed) lap.maxSpeed = static_cast<float>(tp.speed);
    if (tp.heartRate > 0) {
        lap.totalHRM += static_cast<float>(tp.heartRate);
        if (tp.heartRate > lap.maxHRM) lap.maxHRM = static_cast<float>(tp.heartRate);
    }
    if (tp.cadence > 0) {
        lap.totalCadence += static_cast<float>(tp.cadence);
    }
    // lap.totalDistance is finalized (as a delta from lap start) when the
    // lap closes -- see writeLapMessage / newLap below -- since distance on
    // Trackpoint is cumulative-for-the-whole-activity, not per-lap.
}

void FITLogger::newLap(timeData currentTime) {
    uint32_t endFit = toFitTimestamp(currentTime.unixtime());

    if (!laps.empty()) {
        Lap& closingLap = laps.back();
        closingLap.totalDistance = static_cast<float>(_lastDistanceM - _lapStartDistanceM);
        writeLapMessage(closingLap, toFitTimestamp(closingLap.startTime.unixtime()), endFit);
    }

    Lap lap{};
    lap.startTime = currentTime;
    lap.maxHRM = 0;
    lap.totalHRM = 0;
    lap.totalCadence = 0;
    lap.maxSpeed = 0;
    lap.totalDistance = 0;
    lap.parts = 0;
    laps.push_back(lap);

    _lapStartDistanceM = _lastDistanceM;
}

void FITLogger::writeLapMessage(const Lap& lap, uint32_t startTimeFit, uint32_t endTimeFit) {
    uint32_t elapsedSeconds = endTimeFit - startTimeFit;
    uint8_t avgHr = lap.parts > 0 ? static_cast<uint8_t>(lap.totalHRM / lap.parts) : 0xFF;
    uint8_t avgCadence = lap.parts > 0 ? static_cast<uint8_t>(lap.totalCadence / lap.parts) : 0xFF;
    uint8_t maxHr = lap.maxHRM > 0 ? static_cast<uint8_t>(lap.maxHRM) : 0xFF;

    _writer.beginDataMessage(LOCAL_LAP);
    _writer.writeU32(endTimeFit);
    _writer.writeU32(startTimeFit);
    _writer.writeU32(elapsedSeconds * 1000); // scale 1000
    _writer.writeU32(static_cast<uint32_t>(lap.totalDistance * 100)); // scale 100
    _writer.writeU16(static_cast<uint16_t>(lap.maxSpeed * 1000));
    _writer.writeU8(avgHr);
    _writer.writeU8(maxHr);
    _writer.writeU8(avgCadence);
}

void FITLogger::writeSessionMessage(uint32_t endTimeFit) {
    uint32_t elapsedSeconds = endTimeFit - toFitTimestamp(_startTime.unixtime());

    _writer.beginDataMessage(LOCAL_SESSION);
    _writer.writeU32(endTimeFit);
    _writer.writeU32(toFitTimestamp(_startTime.unixtime()));
    _writer.writeU32(elapsedSeconds * 1000);
    _writer.writeU32(static_cast<uint32_t>(_lastDistanceM * 100)); // total = final cumulative value
    _writer.writeU8(2); // sport = cycling
}

void FITLogger::writeActivityMessage(uint32_t endTimeFit) {
    uint32_t totalSeconds = endTimeFit - toFitTimestamp(_startTime.unixtime());

    _writer.beginDataMessage(LOCAL_ACTIVITY);
    _writer.writeU32(endTimeFit);
    _writer.writeU32(totalSeconds * 1000);
    _writer.writeU16(1);  // num_sessions
    _writer.writeU8(0);   // type: 0 = manual
    _writer.writeU8(26);  // event: 26 = activity
    _writer.writeU8(1);   // event_type: 1 = stop
}

bool FITLogger::finaliseLogging() {
    uint32_t endFit = toFitTimestamp(_currentTime.unixtime());

    if (!laps.empty()) {
        Lap& closingLap = laps.back();
        closingLap.totalDistance = static_cast<float>(_lastDistanceM - _lapStartDistanceM);
        writeLapMessage(closingLap, toFitTimestamp(closingLap.startTime.unixtime()), endFit);
    }
    writeSessionMessage(endFit);
    writeActivityMessage(endFit);

    _writer.close();
    return true;
}
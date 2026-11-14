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
    //         speed(u16, scale 1000), heart_rate(u8), cadence(u8), power(u16),
    //         calories(u16, scale 10 = kcal x 10)
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
            {33,  2, FitBaseType::UINT16}, // calories (kcal) -- NOT 15, which is
                                           // not a RECORD field at all
        };
        _writer.writeDefinition(LOCAL_RECORD, FitMesg::RECORD, f, 10);
    }

        // LAP: timestamp(u32), start_time(u32), total_elapsed_time(u32, scale 1000),
    //      total_timer_time(u32, scale 1000), total_distance(u32, scale 100),
    //      total_calories(u16, kcal), avg_speed(u16, scale 1000),
    //      max_speed(u16, scale 1000), avg_heart_rate(u8), max_heart_rate(u8),
    //      avg_cadence(u8)
    //
    // Field numbers verified against the FIT profile (see sim/tools/fit_profile.py).
    // Three of them used to be wrong, which is silent: the file still parsed, the
    // values just landed in the wrong column. 13 is avg_speed (not max_speed),
    // 32 is num_lengths -- a SWIM field -- where total_calories was intended, and
    // 14/max_speed plus 8/total_timer_time were missing entirely.
    {
        FitFieldDef f[] = {
            {253, 4, FitBaseType::UINT32}, // timestamp (= end of lap)
            {2,   4, FitBaseType::UINT32}, // start_time
            {7,   4, FitBaseType::UINT32}, // total_elapsed_time, scale 1000 (s)
            {8,   4, FitBaseType::UINT32}, // total_timer_time, scale 1000 (s)
            {9,   4, FitBaseType::UINT32}, // total_distance, scale 100 (m)
            {11,  2, FitBaseType::UINT16}, // total_calories (kcal)
            {13,  2, FitBaseType::UINT16}, // avg_speed, scale 1000 (m/s)
            {14,  2, FitBaseType::UINT16}, // max_speed, scale 1000 (m/s)
            {15,  1, FitBaseType::UINT8},  // avg_heart_rate (bpm)
            {16,  1, FitBaseType::UINT8},  // max_heart_rate (bpm)
            {17,  1, FitBaseType::UINT8},  // avg_cadence (rpm)
        };
        _writer.writeDefinition(LOCAL_LAP, FitMesg::LAP, f, 11);
    }

    // SESSION: timestamp(u32), start_time(u32), total_elapsed_time(u32,scale1000),
    //          total_timer_time(u32,scale1000), total_distance(u32,scale100),
    //          sport(u8,enum)
    {
        FitFieldDef f[] = {
            {253, 4, FitBaseType::UINT32},
            {2,   4, FitBaseType::UINT32},
            {7,   4, FitBaseType::UINT32},
            {8,   4, FitBaseType::UINT32}, // total_timer_time
            {9,   4, FitBaseType::UINT32},
            {5,   1, FitBaseType::ENUM},   // sport: 2 = cycling
        };
        _writer.writeDefinition(LOCAL_SESSION, FitMesg::SESSION, f, 6);
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
    initTimer(currentTime);
        _lastDistanceM = 0;
    _lapStartDistanceM = 0;
    _lastCalories = 0;
    _lapStartCalories = 0;
    _definitionsWritten = false;
    laps.clear();

    // TODO: match your existing filename scheme (date/time-based, etc.)
    String path = "/activity_" + String(static_cast<unsigned long>(currentTime.unixtime())) + ".fit";
    _open = _writer.open(path);
    if (!_open) {
        // Every write below is a no-op on a closed File32, so without this the
        // whole ride was silently discarded and finaliseLogging() would then
        // seek and patch a header that was never written.
        Serial.print("[FITLogger] failed to open "); Serial.println(path);
        return;
    }
    // Breadcrumb: see TCXLogger::startLogging -- every created log name is
    // logged so a "deleted file reappeared" report can be checked against the
    // names actually written this session.
    Serial.print("[FITLogger] logging to "); Serial.println(path);

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

void FITLogger::addTrackpoint(const Trackpoint& tp, const timeData& currentTime) {
    _currentTime = currentTime;
    uint32_t ts = toFitTimestamp(currentTime.unixtime());

    // altitude field is scale 5, offset 500 per FIT profile: stored = (m + 500) * 5
    uint16_t altitudeEncoded = static_cast<uint16_t>((tp.altitude + 500.0) * 5.0);

    _writer.beginDataMessage(LOCAL_RECORD);
    _writer.writeU32(ts);
    _writer.writeS32(toSemicircles(tp.latitude));
    _writer.writeS32(toSemicircles(tp.longitude));
    _writer.writeU16(altitudeEncoded);
    _writer.writeU32(static_cast<uint32_t>(tp.distance * 100.0));      // cumulative, scale 100
    // FIT speed is m/s at scale 1000; Trackpoint::speed is km/h.
    _writer.writeU16(static_cast<uint16_t>(kmhToMs(tp.speed) * 1000.0));
    _writer.writeU8(tp.heartrate > 0 ? static_cast<uint8_t>(tp.heartrate) : 0xFF);
    _writer.writeU8(tp.cadence > 0 ? static_cast<uint8_t>(tp.cadence) : 0xFF);
        _writer.writeU16(tp.power > 0 ? static_cast<uint16_t>(tp.power) : 0xFFFF);
    // calories is RECORD field 33 (kcal, scale 1). It used to be written as
    // field 15, which is not a RECORD field in the profile at all, so every
    // reader saw it as an unknown column.
        _writer.writeU16(static_cast<uint16_t>(tp.calories));
    _lastCalories = tp.calories;

    _lastDistanceM = tp.distance;

    Lap& lap = laps.back();
    lap.parts++;
    if (tp.speed > lap.maxSpeed) lap.maxSpeed = static_cast<float>(tp.speed);   // km/h, converted on write
    if (tp.heartrate > 0) {
        lap.totalHRM += static_cast<float>(tp.heartrate);
        if (tp.heartrate > lap.maxHRM) lap.maxHRM = static_cast<float>(tp.heartrate);
    }
        if (tp.cadence > 0) {
        lap.totalCadence += static_cast<float>(tp.cadence);
    }
    // Track cumulative calories from FitnessFusion for the lap summary
    lap.totalCalories = tp.calories;
    // lap.totalDistance is finalized (as a delta from lap start) when the
    // lap closes -- see writeLapMessage / newLap below -- since distance on
    // Trackpoint is cumulative-for-the-whole-activity, not per-lap.
}

void FITLogger::newLap(const timeData& currentTime) {
    uint32_t endFit = toFitTimestamp(currentTime.unixtime());

    if (!laps.empty()) {
        Lap& closingLap = laps.back();
        closingLap.totalDistance = static_cast<float>(_lastDistanceM - _lapStartDistanceM);
        // Convert cumulative FitnessFusion calories to this lap's delta, the
        // same way totalDistance is a per-lap delta from an activity-cumulative
        // Trackpoint field.
        closingLap.totalCalories = static_cast<float>(_lastCalories - _lapStartCalories);
        if (closingLap.totalCalories < 0) closingLap.totalCalories = 0;
        // activeLap() still refers to this closing lap: laps.back() has not
        // been replaced yet. Using the banked timer time (instead of the
        // difference of two anchors) keeps the lap duration right even when
        // the ride was paused right up to the lap boundary.
        writeLapMessage(closingLap,
                        static_cast<uint32_t>(activeLap().totalSeconds()),
                        toFitTimestamp(closingLap.startTime.unixtime()), endFit);
    }

    Lap lap{};
    lap.startTime = currentTime;
    lap.activeAtStart = activeAt(currentTime);
    lap.maxHRM = 0;
    lap.totalHRM = 0;
    lap.totalCadence = 0;
    lap.maxSpeed = 0;
        lap.totalDistance = 0;
    lap.parts = 0;
    lap.totalCalories = 0;
    laps.push_back(lap);

    _lapStartDistanceM = _lastDistanceM;
    _lapStartCalories = _lastCalories;

    // Save the _activeAccum value at lap start so writeLapMessage() can
    // compute total_timer_time excluding paused time
    laps.back().activeAccumAtStart = _activeAccum;
}

void FITLogger::writeLapMessage(const Lap& lap, uint32_t elapsedSeconds,
                                uint32_t startTimeFit, uint32_t endTimeFit) {
    uint8_t avgHr = lap.parts > 0 ? static_cast<uint8_t>(lap.totalHRM / lap.parts) : 0xFF;
    uint8_t avgCadence = lap.parts > 0 ? static_cast<uint8_t>(lap.totalCadence / lap.parts) : 0xFF;
    uint8_t maxHr = lap.maxHRM > 0 ? static_cast<uint8_t>(lap.maxHRM) : 0xFF;

    // avg_speed is a real profile field (13) -- it was previously written as if
    // it were max_speed. Derive it from this lap's distance and timer time.
    const float lapMs = kmhToMs(lap.maxSpeed);              // km/h -> m/s
    float avgMs = 0.0f;
    if (elapsedSeconds > 0) avgMs = lap.totalDistance / static_cast<float>(elapsedSeconds);

    _writer.beginDataMessage(LOCAL_LAP);
    _writer.writeU32(endTimeFit);
    _writer.writeU32(startTimeFit);
    // total_elapsed_time includes pauses; total_timer_time excludes pauses.
    // Compute paused time during this lap by comparing _activeAccum at lap start
    // (saved in lap.activeAccumAtStart by newLap()) with the current value.
    uint32_t lapPausedSeconds = 0;
    if (elapsedSeconds > 0) {
        timeDuration pausedDiff = _activeAccum - lap.activeAccumAtStart;
        lapPausedSeconds = static_cast<uint32_t>(pausedDiff.totalSeconds());
        if (lapPausedSeconds > elapsedSeconds) {
            lapPausedSeconds = elapsedSeconds;  // clamp if needed
        }
    }
    _writer.writeU32(elapsedSeconds * 1000);        // total_elapsed_time, scale 1000 (includes pauses)
    _writer.writeU32((elapsedSeconds - lapPausedSeconds) * 1000);  // total_timer_time, scale 1000 (excludes pauses)
    _writer.writeU32(static_cast<uint32_t>(lap.totalDistance * 100)); // scale 100
    _writer.writeU16(static_cast<uint16_t>(lap.totalCalories));       // kcal, scale 1
    _writer.writeU16(static_cast<uint16_t>(avgMs * 1000.0f));         // avg_speed, scale 1000
    _writer.writeU16(static_cast<uint16_t>(lapMs * 1000.0f));         // max_speed, scale 1000
    _writer.writeU8(avgHr);
    _writer.writeU8(maxHr);
        _writer.writeU8(avgCadence);
}

void FITLogger::writeSessionMessage(uint32_t endTimeFit) {
    // Timer time from the logger's own accumulator, NOT from the difference of
    // the time anchors: a pause at the end of the ride (auto-pause on a stop)
    // used to make that difference zero, which is how a 10 s ride was written
    // with total_elapsed_time = 0.
    uint32_t elapsedSeconds = static_cast<uint32_t>(activeTotal().totalSeconds());

    _writer.beginDataMessage(LOCAL_SESSION);
    _writer.writeU32(endTimeFit);
    _writer.writeU32(toFitTimestamp(_startTime.unixtime()));
    _writer.writeU32(elapsedSeconds * 1000);
    _writer.writeU32(elapsedSeconds * 1000); // total_timer_time
    _writer.writeU32(static_cast<uint32_t>(_lastDistanceM * 100)); // total = final cumulative value
    _writer.writeU8(2); // sport = cycling
}

void FITLogger::writeActivityMessage(uint32_t endTimeFit) {
    uint32_t totalSeconds = static_cast<uint32_t>(activeTotal().totalSeconds());

    _writer.beginDataMessage(LOCAL_ACTIVITY);
    _writer.writeU32(endTimeFit);
    _writer.writeU32(totalSeconds * 1000);
    _writer.writeU16(1);  // num_sessions
    _writer.writeU8(0);   // type: 0 = manual
    _writer.writeU8(26);  // event: 26 = activity
    _writer.writeU8(1);   // event_type: 1 = stop
}

bool FITLogger::finaliseLogging() {
    if (!_open) return false;   // nothing was ever written

    uint32_t endFit = toFitTimestamp(_currentTime.unixtime());

    if (!laps.empty()) {
        Lap& closingLap = laps.back();
        closingLap.totalDistance = static_cast<float>(_lastDistanceM - _lapStartDistanceM);
        closingLap.totalCalories = static_cast<float>(_lastCalories - _lapStartCalories);
        if (closingLap.totalCalories < 0) closingLap.totalCalories = 0;
        writeLapMessage(closingLap,
                        static_cast<uint32_t>(activeLap().totalSeconds()),
                        toFitTimestamp(closingLap.startTime.unixtime()), endFit);
    }
    writeSessionMessage(endFit);
    writeActivityMessage(endFit);

    _writer.close();
    _open = false;
    return true;
}
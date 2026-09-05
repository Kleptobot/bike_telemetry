#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <time.h>

#include "SensorData.hpp"

// =============================================================================
// MeasurementFrame -- the HAL/App contract's input side.
//
// One complete, factual snapshot of everything acquired, rebuilt at the end
// of every HAL::update() (and of the simulator's replacement of it).
//
// House rules (deliberate, see the HAL/App boundary notes):
//   1. Every field here is ACQUIRED data -- a fact about the world. Derived
//      values (fused altitude, chosen speed, grade, distance) belong to the
//      App-side fusion layer, never here.
//   2. Completeness over curation: if a driver produces a fact, the frame
//      carries it. Adding a field is non-breaking; HAL does not get to
//      decide what the App is allowed to care about.
//   3. Every periodic channel is stamped with when it was captured (tsMs),
//      a monotonically increasing sequence number (seq, +1 per accepted
//      sample), and the OBSERVED interval between the two most recent
//      accepted samples (dtMs -- which is not the nominal period when the
//      I2C bus was busy; that skew is information, so it is surfaced).
//   4. "Could two honest implementations acquire this differently?" -> fact
//      -> frame.  "Do they agree but might derive differently?" -> conclusion
//      -> fusion layer.
// =============================================================================

// Uniform wrapper for a periodically sampled channel.
template <typename T>
struct Sample {
    T        value{};
    bool     valid{false};
    uint32_t tsMs{0};    // millis() when captured
    uint16_t seq{0};     // +1 per accepted sample; consumers test != against the previous frame
    uint16_t dtMs{0};    // observed interval between the two most recent accepted samples; 0 = unknown
};

template <typename T>
inline void fillSample(Sample<T>& dst, T value, bool valid, uint32_t tsMs, uint16_t seq, uint16_t dtMs) {
    dst.value = value;
    dst.valid = valid;
    dst.tsMs  = tsMs;
    dst.seq   = seq;
    dst.dtMs  = dtMs;
}

// BLE spec event-counted channel (CSC wheel/crank revolutions): the native
// counters and event clock, NOT a rate smeared off them. Deriving speed or
// distance from these is a fusion-layer job; counter delta over event-time
// delta is the exact answer with no main-loop sampling aliasing.
struct EventCount {
    uint32_t counter{0};       // cumulative revolutions (free-running; subtract to difference)
    uint16_t evtTime1024{0};   // last native event time, 1/1024 s units per the BLE spec
    bool     valid{false};
    uint32_t tsMs{0};          // millis() of the notification that carried this event
    uint16_t seq{0};           // low 16 bits of counter: changes on every revolution
    uint16_t dtMs{0};          // 0: use the sensor's own 1/1024 s event clock instead
};

inline void fillEventCount(EventCount& dst, uint32_t counter, uint16_t evtTime1024,
                           bool valid, uint32_t tsMs) {
    dst.counter     = counter;
    dst.evtTime1024 = evtTime1024;
    dst.valid       = valid;
    dst.tsMs        = tsMs;
    dst.seq         = (uint16_t)counter;
    dst.dtMs        = 0;
}

// --- GNSS --------------------------------------------------------------------
// TinyGPS++ commits its sub-objects (location, altitude, speed, ...) on
// different sentences, so each gets its own freshness tracking. A commit is
// detected the same way the existing altitude path detects one: age() shrinks.
class AgeSeqTracker {
    public:
    // Call once per tick with the element's current age(). Returns true when
    // a newer value was committed since the previous tick.
    bool note(uint32_t ageMs, uint32_t nowMs) {
        if (ageMs < _prevAge) {
            if (_lastMs != 0 && nowMs > _lastMs) {
                const uint32_t dt = nowMs - _lastMs;
                _dtMs = dt > 65000 ? 65000u : (uint16_t)dt;
            } else {
                _dtMs = 0;
            }
            _lastMs  = nowMs;
            ++_seq;
            _prevAge = ageMs;
            return true;
        }
        return false;
    }
    uint16_t seq()    const { return _seq; }
    uint16_t dtMs()   const { return _dtMs; }
    uint32_t lastMs() const { return _lastMs; }

    private:
    uint32_t _prevAge = 0xFFFFFFFFul;   // TinyGPS++ reports 0xFFFFFFFF for "never committed"
    uint32_t _lastMs  = 0;
    uint16_t _seq     = 0;
    uint16_t _dtMs    = 0;
};

// Commit timestamp for a TinyGPS++ sub-object: millis() minus its age().
inline uint32_t gpsAgeToTs(bool valid, uint32_t ageMs, uint32_t nowMs) {
    if (!valid || ageMs >= nowMs) return 0;
    return nowMs - ageMs;
}

struct GpsSample {
    double   lat{0}, lng{0};
    bool     valid{false};
    uint32_t tsMs{0};
    uint16_t seq{0};
    uint16_t dtMs{0};
};

struct GpsTrackers {
    AgeSeqTracker loc, alt, spd, crs, hdop, sats, utcTime, utcDate;
};

// --- The frame ---------------------------------------------------------------

struct MeasurementFrame {
    uint32_t tickStartMs{0};   // millis() at the top of the HAL tick that built this frame
    uint16_t frameSeq{0};      // +1 per HAL tick

    // Motion / environment
    Sample<imu_data> imu;            // full 6-DOF: accel xyz + gyro xyz (Sensors reads all six)
    Sample<float>    baroPressurePa;
    Sample<float>    baroTempC;
    Sample<float>    rtcTempC;       // RTC die temperature (historically smuggled inside dps_data)
    Sample<bool>     charging;       // BAT_CHARGE_STATE input: true while the charger reports charging

    // GNSS
    GpsSample        gpsPos;
    // TinyGPS++ presents every derived quantity as double; the frame keeps
    // that width so no precision is silently narrowed at the boundary.
    Sample<double>   gpsAltM;
    Sample<double>   gpsSpeedKmh;
    Sample<double>   gpsCourseDeg;   // parsed by TinyGPS++ on every RMC; previously unreachable
    Sample<double>   gpsHdop;
    Sample<uint8_t>  gpsSatsUsed;    // parsed on every GGA; previously unreachable
    Sample<uint32_t> gpsUtcTimeHMS;  // RMC time-of-day packed hhmmss (representation, not derivation)
    Sample<uint32_t> gpsUtcDateYMD;  // RMC date packed yyyymmdd

    // Drivetrain
    EventCount wheelRevs;            // exact cumulative wheel revolutions (primary CSC device)
    EventCount crankRevs;            // exact cumulative crank revolutions (primary CSC device)
    // Legacy aggregates: filtered and averaged below the boundary inside the
    // CSC parser (documented provenance). Kept so migration is incremental;
    // the raw counters above are the future-facing source.
    data_record wheelRpmFiltered{};
    data_record cadenceRpmFiltered{};

    // Power / bio / electrical
    data_record powerWatts{};        // cps parser
    data_record torqueNm{};          // decoded today, previously dropped at the boundary
    data_record pedalBalancePct{};   // ditto
    data_record forceMagN{};         // ditto
    data_record heartRateBpm{};      // hrm parser
    Sample<float> vbatVolts;         // the actual cell voltage; the percentage is derived from it
    int16_t       batteryPct{-1};    // legacy derived gauge (mirrors getBatteryPercentage())

    // Time
    Sample<time_t> rtcNow;           // PCF85063A epoch seconds (sensorSystem.now())

    // Storage (presence/mount are facts App already polls today)
    bool sdPresent{false};
    bool sdMounted{false};
};

// Fill the frame's GNSS section from a TinyGPSPlus that has been fed NMEA.
// Shared by the firmware HAL and the simulator, which both own the same real
// parser state.
//
// TinyGPSPlus& is deliberately non-const: the library's value accessors
// (month(), day(), hour(), ...) are not const-qualified, so a const reference
// cannot call them.
inline void fillGps(MeasurementFrame& f, TinyGPSPlus& gps, uint32_t nowMs, GpsTrackers& trk) {
    const bool locValid = gps.location.isValid();
    f.gpsPos.lat   = gps.location.lat();
    f.gpsPos.lng   = gps.location.lng();
    f.gpsPos.valid = locValid;
    f.gpsPos.tsMs  = gpsAgeToTs(locValid, gps.location.age(), nowMs);
    trk.loc.note(gps.location.age(), nowMs);
    f.gpsPos.seq   = trk.loc.seq();
    f.gpsPos.dtMs  = trk.loc.dtMs();

    trk.alt.note(gps.altitude.age(), nowMs);
    fillSample(f.gpsAltM, gps.altitude.meters(), gps.altitude.isValid(),
               gpsAgeToTs(gps.altitude.isValid(), gps.altitude.age(), nowMs),
               trk.alt.seq(), trk.alt.dtMs());

    trk.spd.note(gps.speed.age(), nowMs);
    fillSample(f.gpsSpeedKmh, gps.speed.kmph(), gps.speed.isValid(),
               gpsAgeToTs(gps.speed.isValid(), gps.speed.age(), nowMs),
               trk.spd.seq(), trk.spd.dtMs());

    trk.crs.note(gps.course.age(), nowMs);
    fillSample(f.gpsCourseDeg, gps.course.deg(), gps.course.isValid(),
               gpsAgeToTs(gps.course.isValid(), gps.course.age(), nowMs),
               trk.crs.seq(), trk.crs.dtMs());

    trk.hdop.note(gps.hdop.age(), nowMs);
    fillSample(f.gpsHdop, gps.hdop.hdop(), gps.hdop.isValid(),
               gpsAgeToTs(gps.hdop.isValid(), gps.hdop.age(), nowMs),
               trk.hdop.seq(), trk.hdop.dtMs());

    trk.sats.note(gps.satellites.age(), nowMs);
    fillSample(f.gpsSatsUsed, (uint8_t)gps.satellites.value(), gps.satellites.isValid(),
               gpsAgeToTs(gps.satellites.isValid(), gps.satellites.age(), nowMs),
               trk.sats.seq(), trk.sats.dtMs());

    trk.utcTime.note(gps.time.age(), nowMs);
    fillSample(f.gpsUtcTimeHMS,
               (uint32_t)gps.time.hour() * 10000u + (uint32_t)gps.time.minute() * 100u
                                                          + (uint32_t)gps.time.second(),
               gps.time.isValid(),
               gpsAgeToTs(gps.time.isValid(), gps.time.age(), nowMs),
               trk.utcTime.seq(), trk.utcTime.dtMs());

    trk.utcDate.note(gps.date.age(), nowMs);
    fillSample(f.gpsUtcDateYMD,
               (uint32_t)gps.date.year() * 10000u + (uint32_t)gps.date.month() * 100u
                                                          + (uint32_t)gps.date.day(),
               gps.date.isValid(),
               gpsAgeToTs(gps.date.isValid(), gps.date.age(), nowMs),
               trk.utcDate.seq(), trk.utcDate.dtMs());
}

#endif /* MEASUREMENTS_H */
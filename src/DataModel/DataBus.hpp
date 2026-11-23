#ifndef DATABUS_H
#define DATABUS_H

#include <Arduino.h>

// =============================================================================
// DataBus -- a lightweight pub/sub hub for derived signals.
//
// Design goals for the embedded target (nRF52840, 232 KB RAM):
//   - Fixed subscriber slots per topic: no dynamic allocation after setup.
//   - Type-safe access: each topic has a known data type.
//   - Synchronous dispatch: subscribers are invoked at publish() time.
//   - Backwards-compatible polling: get<T>(topic) returns the latest value.
//
// This is Phase 1: the DataBus owns the new fusion-derived channels
// (altitude, vario, speed, grade, cadence, gear ratio, energy, acceleration,
// estimated power, total ascent/descent, coasting). Existing DataModel
// providers are untouched; migration is incremental.
// =============================================================================

// Topic identifiers for all data channels the bus can carry.
enum class Topic : uint8_t {
    // Fusion outputs
    FusedAltitude,
    Vario,                 // float: vertical velocity (m/s)
    SelectedSpeed,
    SmoothedGrade,
    Cadence,
    GearRatio,
    Energy,
    Acceleration,
    EstimatedPower,
    TotalAscent,
    TotalDescent,
    Coasting,

    // New derived signals (batch: timing, display power, analytics)
    ElapsedSec,      // float: seconds since logging started (incl. pauses)
    MovingSec,       // float: seconds actively logging (pauses excluded)
    Avg3sPower,      // float: 3 s rolling average power (meter or estimate)
    WattsPerKg,      // float: current power / rider mass
    Vam,             // float: vertical ascent rate, m/h
    BatteryCharging, // bool: charger reports charging (debounced)
    GpsQuality,      // uint8_t: 0 = Good, 1 = Fair, 2 = Poor
    ShiftCount,      // uint32_t: detected gear shifts this ride
    GearShifted,     // bool: shift event on this tick

    // Fitness (from FitnessFusion; ride-scoped, reset with distance)
    Calories,        // float: kcal accumulated this ride
    NormalizedPower, // float: 30 s rolling NP (power meter), watts
    IntensityFactor, // float: NP / FTP
    Tss,             // float: accumulated training stress score
    HrZone,          // uint8_t: 0 = none, else 1..5
    PowerZone,       // uint8_t: 0 = none/FTP unset, else 1..5

    // Position / distance
    GpsValid,        // bool: current GPS fix validity
    Location,        // location_data: valid + lat/lng (24 bytes, fits a slot)
    DistanceDeltaM,  // float: per-tick distance increment (metres)
    TotalDistanceM,  // float: accumulated ride distance (metres)
    GpsSpeed,        // float: GPS-reported ground speed (km/h)
    GpsAltitude,     // float: GPS-reported altitude (metres)
    GpsCourse,       // float: GPS course over ground (degrees)
    GpsSats,         // uint8_t: satellites used in the fix
    GpsHdop,         // float: horizontal dilution of precision
    BaroAlt,         // float: raw baro altitude from P0_local, before bias trim

    // Raw onboard IMU channels
    ImuValid,        // bool: at least one successful IMU sample has been acquired
    ImuAccelX,       // float: acceleration in g
    ImuAccelY,       // float: acceleration in g
    ImuAccelZ,       // float: acceleration in g
    ImuGyroX,        // float: angular velocity in degrees/second
    ImuGyroY,        // float: angular velocity in degrees/second
    ImuGyroZ,        // float: angular velocity in degrees/second

    // Power-meter extensions (decoded by the CPS parser, previously dropped)
    PedalBalance,    // float: left/right pedal balance (percent left)
    TorqueNm,        // float: pedal torque (newton-metres)

    // System channels
    Battery,         // int16_t: percent
    BatteryVolts,    // float: raw cell voltage
    Temperature,     // float: celsius
    HeartRate,       // float: bpm
    PowerMeter,      // float: watts

    TopicCount  // Must be last; used for array sizing
};

enum class TopicValueKind : uint8_t {
    Float,
    Bool,
    UInt8,
    UInt32,
    Int16,
    Location
};

enum class TopicDisplayFormat : uint8_t {
    Number,
    ElapsedTime,
    MovingTime,
    Location,
    BooleanStatus,
    CoastingStatus,
    HrZone,
    PowerZone,
    GpsQuality
};

enum class TopicColor : uint8_t {
    White,
    Green,
    Orange,
    Red
};

// The DataBus topic catalog is the source of truth for display and CSV
// interpretation as well as storage type. The catalog order is also the
// stable CSV column order (time is emitted separately).
struct TopicMetadata {
    Topic topic;
    const char* displayName;
    const char* units;
    const char* csvName;
    TopicValueKind valueKind;
    TopicDisplayFormat displayFormat;
    uint8_t decimals;
    float displayScale;
    const char* csvName2 = nullptr;
    const char* csvDeferredName = nullptr;
    const char* trueText = "ON";
    const char* falseText = "OFF";
    const char* qualityGoodText = "GOOD";
    const char* qualityFairText = "FAIR";
    const char* qualityPoorText = "POOR";
    char zonePrefix = 'Z';
    TopicColor trueColor = TopicColor::Green;
    TopicColor falseColor = TopicColor::Red;
    TopicColor qualityGoodColor = TopicColor::Green;
    TopicColor qualityFairColor = TopicColor::Orange;
    TopicColor qualityPoorColor = TopicColor::Red;
};

constexpr TopicMetadata numericTopic(Topic topic, const char* name,
                                     const char* units, const char* csvName,
                                     TopicValueKind valueKind = TopicValueKind::Float,
                                     uint8_t decimals = 1, float displayScale = 1.0f) {
    return {topic, name, units, csvName, valueKind, TopicDisplayFormat::Number,
            decimals, displayScale};
}

constexpr TopicMetadata timedTopic(Topic topic, const char* name,
                                   const char* csvName, TopicDisplayFormat format) {
    return {topic, name, "", csvName, TopicValueKind::Float, format, 0, 1.0f};
}

constexpr TopicMetadata locationTopic() {
    TopicMetadata info{Topic::Location, "Location", "", "latitude",
                       TopicValueKind::Location, TopicDisplayFormat::Location,
                       1, 1.0f};
    info.csvName2 = "longitude";
    info.csvDeferredName = "locationvalid";
    return info;
}

constexpr TopicMetadata booleanTopic(Topic topic, const char* name,
                                    const char* csvName, const char* trueText,
                                    const char* falseText, TopicColor trueColor,
                                    TopicColor falseColor,
                                    TopicDisplayFormat format = TopicDisplayFormat::BooleanStatus) {
    TopicMetadata info{topic, name, "", csvName, TopicValueKind::Bool, format, 0, 1.0f};
    info.trueText = trueText;
    info.falseText = falseText;
    info.trueColor = trueColor;
    info.falseColor = falseColor;
    return info;
}

constexpr TopicMetadata zoneTopic(Topic topic, const char* name,
                                  const char* csvName, char prefix) {
    TopicMetadata info{topic, name, "", csvName, TopicValueKind::UInt8,
                       prefix == 'Z' ? TopicDisplayFormat::HrZone : TopicDisplayFormat::PowerZone,
                       0, 1.0f};
    info.zonePrefix = prefix;
    return info;
}

constexpr TopicMetadata gpsQualityTopic() {
    return {Topic::GpsQuality, "GPSQuality", "", "gpsquality",
            TopicValueKind::UInt8, TopicDisplayFormat::GpsQuality, 0, 1.0f,
            nullptr, nullptr, "ON", "OFF", "GOOD", "FAIR", "POOR", 'Z',
            TopicColor::Green, TopicColor::Red, TopicColor::Green,
            TopicColor::Orange, TopicColor::Red};
}

// Keep this catalog complete with Topic. Its order preserves the familiar CSV
// columns first and also defines the type-selection order in DisplayEdit.
static constexpr TopicMetadata kTopicMetadata[] = {
    locationTopic(),
    numericTopic(Topic::FusedAltitude, "Altitude", "m  ", "altitude"),
    numericTopic(Topic::SelectedSpeed, "Speed", "km/h", "speed"),
    numericTopic(Topic::HeartRate, "HeartRate", "bpm", "heartrate"),
    numericTopic(Topic::Cadence, "Cadence", "rpm", "cadence"),
    numericTopic(Topic::PowerMeter, "Power", "W  ", "power"),
    numericTopic(Topic::Calories, "Calories", "kcal", "calories", TopicValueKind::Float, 0),
    numericTopic(Topic::NormalizedPower, "NP", "W  ", "normalizedpower", TopicValueKind::Float, 0),
    numericTopic(Topic::IntensityFactor, "IF", "x  ", "intensityfactor", TopicValueKind::Float, 2),
    numericTopic(Topic::Tss, "TSS", "pts", "tss", TopicValueKind::Float, 0),
    zoneTopic(Topic::HrZone, "HrZone", "hrzone", 'Z'),
    zoneTopic(Topic::PowerZone, "PowerZone", "powerzone", 'P'),
    numericTopic(Topic::TotalDistanceM, "TotalDist", "km ", "distance", TopicValueKind::Float, 1, 0.001f),

    numericTopic(Topic::Vario, "Vario", "m/s", "vario"),
    numericTopic(Topic::SmoothedGrade, "Grade", "%  ", "grade"),
    numericTopic(Topic::GearRatio, "GearRatio", "x  ", "gearratio", TopicValueKind::Float, 2),
    numericTopic(Topic::Energy, "Energy", "kJ ", "energy"),
    numericTopic(Topic::Acceleration, "Accel", "m/s2", "acceleration", TopicValueKind::Float, 2),
    numericTopic(Topic::EstimatedPower, "EstPower", "W~ ", "estimatedpower"),
    numericTopic(Topic::TotalAscent, "Ascent", "up ", "totalascent", TopicValueKind::Float, 0),
    numericTopic(Topic::TotalDescent, "Descent", "dn ", "totaldescent", TopicValueKind::Float, 0),
    booleanTopic(Topic::Coasting, "Coasting", "coasting", "COAST", "PEDAL",
                 TopicColor::Orange, TopicColor::Green, TopicDisplayFormat::CoastingStatus),
    timedTopic(Topic::ElapsedSec, "Elapsed", "elapsedsec", TopicDisplayFormat::ElapsedTime),
    timedTopic(Topic::MovingSec, "Moving", "movingsec", TopicDisplayFormat::MovingTime),
    numericTopic(Topic::Avg3sPower, "AvgPwr", "W  ", "avg3spower"),
    numericTopic(Topic::WattsPerKg, "W/kg", "    ", "wattsperkg"),
    numericTopic(Topic::Vam, "VAM", "m/h ", "vam", TopicValueKind::Float, 0),
    booleanTopic(Topic::BatteryCharging, "Charging", "batterycharging", "CHG", "IDLE",
                 TopicColor::Orange, TopicColor::White),
    gpsQualityTopic(),
    numericTopic(Topic::ShiftCount, "ShiftCount", "shft", "shiftcount", TopicValueKind::UInt32, 0),
    booleanTopic(Topic::GearShifted, "ShiftEvent", "gearshifted", "SHIFT", "-----",
                 TopicColor::Orange, TopicColor::White),
    booleanTopic(Topic::GpsValid, "GPSValid", "gpsvalid", "FIX", "NOFIX",
                 TopicColor::Green, TopicColor::Red),
    numericTopic(Topic::DistanceDeltaM, "Distance", "m  ", "distancedelta"),
    numericTopic(Topic::GpsSpeed, "GpsSpeed", "km/h", "gpsspeed"),
    numericTopic(Topic::GpsAltitude, "GpsAlt", "m  ", "gpsaltitude"),
    numericTopic(Topic::GpsCourse, "GpsCourse", "deg", "gpscourse", TopicValueKind::Float, 0),
    numericTopic(Topic::GpsSats, "GpsSats", "sat", "gpssats", TopicValueKind::UInt8, 0),
    numericTopic(Topic::GpsHdop, "GpsHdop", "dop", "gpshdop", TopicValueKind::Float, 2),
    numericTopic(Topic::BaroAlt, "BaroAlt", "m  ", "baroalt"),
    booleanTopic(Topic::ImuValid, "IMUValid", "imuvalid", "READY", "N/A",
                 TopicColor::Green, TopicColor::Red),
    numericTopic(Topic::ImuAccelX, "AccelX", "g   ", "imuaccelx"),
    numericTopic(Topic::ImuAccelY, "AccelY", "g   ", "imuaccely"),
    numericTopic(Topic::ImuAccelZ, "AccelZ", "g   ", "imuaccelz"),
    numericTopic(Topic::ImuGyroX, "GyroX", "d/s ", "imugyrox"),
    numericTopic(Topic::ImuGyroY, "GyroY", "d/s ", "imugyroy"),
    numericTopic(Topic::ImuGyroZ, "GyroZ", "d/s ", "imugyroz"),
    numericTopic(Topic::PedalBalance, "PedalBalance", "%L ", "pedalbalance"),
    numericTopic(Topic::TorqueNm, "Torque", "Nm ", "torque", TopicValueKind::Float, 2),
    numericTopic(Topic::Battery, "Battery", "%  ", "battery", TopicValueKind::Int16, 0),
    numericTopic(Topic::BatteryVolts, "BattVolts", "V  ", "batteryvolts", TopicValueKind::Float, 2),
    numericTopic(Topic::Temperature, "Temperature", "C  ", "temperature"),
};

static constexpr size_t kTopicMetadataCount =
    sizeof(kTopicMetadata) / sizeof(kTopicMetadata[0]);

constexpr bool everyTopicHasOneMetadataEntry() {
    const size_t topicCount = static_cast<size_t>(Topic::TopicCount);
    if (kTopicMetadataCount != topicCount) return false;
    for (size_t topic = 0; topic < topicCount; ++topic) {
        size_t matches = 0;
        for (size_t i = 0; i < kTopicMetadataCount; ++i) {
            if (static_cast<size_t>(kTopicMetadata[i].topic) == topic) ++matches;
        }
        if (matches != 1) return false;
    }
    return true;
}

static_assert(everyTopicHasOneMetadataEntry(),
              "Every DataBus topic must have exactly one metadata entry.");

inline const TopicMetadata* topicMetadata(Topic topic) {
    for (const TopicMetadata& info : kTopicMetadata) {
        if (info.topic == topic) return &info;
    }
    return nullptr;
}

inline const char* topicDisplayName(Topic topic) {
    const TopicMetadata* info = topicMetadata(topic);
    return info ? info->displayName : "-";
}

inline Topic topicFromDisplayName(const String& name) {
    for (const TopicMetadata& info : kTopicMetadata) {
        if (name == info.displayName) return info.topic;
    }
    return Topic::TopicCount;
}

inline Topic nextDisplayTopic(Topic topic) {
    size_t i = 0;
    for (; i < kTopicMetadataCount; ++i) {
        if (kTopicMetadata[i].topic == topic) break;
    }
    return kTopicMetadata[i < kTopicMetadataCount
                              ? (i + 1) % kTopicMetadataCount
                              : 0].topic;
}

inline Topic previousDisplayTopic(Topic topic) {
    size_t i = 0;
    for (; i < kTopicMetadataCount; ++i) {
        if (kTopicMetadata[i].topic == topic) break;
    }
    return kTopicMetadata[i < kTopicMetadataCount
                              ? (i + kTopicMetadataCount - 1) % kTopicMetadataCount
                              : kTopicMetadataCount - 1].topic;
}

// Maximum subscribers per topic. Fixed at compile time to avoid heap allocation.
static constexpr size_t MAX_SUBSCRIBERS_PER_TOPIC = 4;

// Subscriber callback signature. Receives a pointer to the stored data
// and an optional context pointer. Callers must cast the void* to the
// correct type for the topic they subscribed to.
using SubscriberCallback = void (*)(const void* data, void* ctx);

// Storage for a topic's latest value and its subscribers.
struct TopicSlot {
            alignas(double) unsigned char storage[64];  // Enough for any current data type (location_data with doubles needs >32)
    size_t size = 0;                           // Actual size of stored type
    uint32_t sequence = 0;                     // Incremented on each publish
    bool valid = false;                        // Whether data has been published

    SubscriberCallback subscribers[MAX_SUBSCRIBERS_PER_TOPIC] = {};
    void* subscriberContexts[MAX_SUBSCRIBERS_PER_TOPIC] = {};
    uint8_t subscriberCount = 0;
};

class DataBus {
public:
    DataBus() = default;

    // Publish new data to a topic. All registered subscribers are invoked
    // synchronously. The value is copied into internal storage.
    template<typename T>
    void publish(Topic topic, const T& data) {
        TopicSlot& slot = _slots[static_cast<size_t>(topic)];
        *reinterpret_cast<T*>(slot.storage) = data;
        slot.size = sizeof(T);
        slot.valid = true;
        slot.sequence++;

        for (uint8_t i = 0; i < slot.subscriberCount; i++) {
            slot.subscribers[i](slot.storage, slot.subscriberContexts[i]);
        }
    }

    // Get the latest value published to a topic.
    // Returns a default-constructed T if the topic has never been published.
    template<typename T>
    const T& get(Topic topic) const {
        const TopicSlot& slot = _slots[static_cast<size_t>(topic)];
        if (slot.valid && slot.size == sizeof(T)) {
            return *reinterpret_cast<const T*>(slot.storage);
        }
        static const T defaultVal{};
        return defaultVal;
    }

    // Check if a topic has received at least one publish.
    bool isValid(Topic topic) const {
        return _slots[static_cast<size_t>(topic)].valid;
    }

    // Get the current sequence number for a topic. Useful for change detection:
    // consumers can cache the last sequence they processed and skip if unchanged.
    uint32_t sequence(Topic topic) const {
        return _slots[static_cast<size_t>(topic)].sequence;
    }

    // Subscribe to a topic with a callback and optional context pointer.
    // The callback receives a pointer to the stored data and the context.
    // Returns true if subscription succeeded, false if subscriber slots are full.
    bool subscribe(Topic topic, SubscriberCallback callback, void* ctx = nullptr) {
        TopicSlot& slot = _slots[static_cast<size_t>(topic)];
        if (slot.subscriberCount >= MAX_SUBSCRIBERS_PER_TOPIC) {
            return false;
        }
        slot.subscribers[slot.subscriberCount] = callback;
        slot.subscriberContexts[slot.subscriberCount] = ctx;
        slot.subscriberCount++;
        return true;
    }

    // Remove a previously added subscriber. Matches on both callback and ctx
    // (ctx is typically the subscriber object's this pointer, which is unique).
    // Returns true if a matching subscription was found and removed.
    bool unsubscribe(Topic topic, SubscriberCallback callback, void* ctx = nullptr) {
        TopicSlot& slot = _slots[static_cast<size_t>(topic)];
        for (uint8_t i = 0; i < slot.subscriberCount; i++) {
            if (slot.subscribers[i] == callback && slot.subscriberContexts[i] == ctx) {
                // Swap-with-last removal keeps the array compact. Order of
                // delivery between subscribers is not guaranteed anyway.
                const uint8_t last = slot.subscriberCount - 1;
                slot.subscribers[i]           = slot.subscribers[last];
                slot.subscriberContexts[i]    = slot.subscriberContexts[last];
                slot.subscriberCount--;
                return true;
            }
        }
        return false;
    }

private:
    TopicSlot _slots[static_cast<size_t>(Topic::TopicCount)];
};

#endif /* DATABUS_H */

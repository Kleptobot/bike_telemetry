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
    Vario,
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

#ifndef FITNESSFUSION_H
#define FITNESSFUSION_H

#include <Arduino.h>
#include <time.h>
#include "HAL/Measurements.hpp"
#include "DataModel/BioDataProvider.hpp"

// =============================================================================
// FitnessFusion -- the rider-facing fitness interpretation stage, owned by
// FusionEngine (the same composed pattern as AltitudeFusion).
//
// Where the rest of the fusion layer answers "what did the world do" (speed,
// altitude, power), this stage answers "what did that mean for the rider":
//   - live calories, from the best source each second (power meter,
//     physics estimate, then heart rate) with the standard ~24 % gross
//     efficiency conversion from mechanical kJ,
//   - heart-rate zone (1-5) from the rider's five zone starts, plus the
//     accumulated seconds in each zone,
//   - power zone (1-5) from FTP bands (55/75/90/105 %),
//   - 30 s rolling Normalized Power (power meter only),
//   - Intensity Factor = NP / FTP, and accumulated TSS = IF^2 * hours * 100.
//
// It is pure state + one update() call per App tick; it owns no I/O and is
// reset() exactly when the ride distance resets (logging start), so its
// accumulators always describe the current ride.
// =============================================================================

// Rider/bike configuration fed in by App each tick. Cheap value semantics:
// the source of truth stays in DataModel (BioDataProvider / BikeDataProvider),
// this is the per-tick snapshot the fusion pipeline actually uses.
struct FitnessProfile {
    float riderMassKg      = 75.0f;
    float bikeMassKg       = 10.0f;
    float ageYears         = 30.0f;
    CaloricProfile caloricProfile = CaloricProfile::Other;
    // Heart-rate zone start thresholds (bpm), zone1 .. zone5.
    float zoneStartsBpm[5] = { 55.0f, 117.0f, 138.0f, 151.0f, 162.0f };
    uint16_t ftpWatts      = 200;  // FTP; 0 disables power-zone / IF / TSS

    float totalMassKg() const { return riderMassKg + bikeMassKg; }
};

// Accumulated fitness channels for the current ride. Copied into
// DerivedChannels by FusionEngine each tick and published to the DataBus.
struct FitnessOutput {
    float   caloriesKcal    = 0.0f;
    uint8_t hrZone          = 0;   // 0 = no HR / below zone 1; else 1..5
    uint8_t powerZone       = 0;   // 0 = no power / FTP unset; else 1..5
    float   normalizedPowerW = 0.0f;   // 30 s rolling NP
    float   intensityFactor  = 0.0f;   // NP / FTP
    float   tss              = 0.0f;   // accumulated training stress score
    // Seconds accumulated in each HR zone: index 0..4 == zone1..zone5.
    float   timeInZoneSec[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
};

class FitnessFusion {
public:
    // One call per App tick, after the speed/est-power stages have run.
    //   dtSec        observed RTC interval this tick, already sanity-clamped
    //                (0 when no usable interval -- the stage then does nothing)
    //   powerWatts   power-meter watts (0 when no meter is live)
    //   estPowerWatts physics estimate (0 when not computable -- e.g. no baro)
    //   speedKmh     selected speed (reserved for future source heuristics)
    void update(const MeasurementFrame& f, const FitnessProfile& profile,
                float dtSec, float powerWatts, float estPowerWatts);

    // Zero all ride accumulators. Called from FusionEngine::resetDistance(),
    // i.e. exactly when ride distance resets (logging start / stop).
    void reset();

    const FitnessOutput& out() const { return _out; }

private:
    static constexpr int   HR_ZONES      = 5;
    // Gross efficiency (mechanical work -> dietary calories), dimensionless.
    static constexpr float EFFICIENCY    = 0.24f;
    // kcal per kJ.
    static constexpr float KJ_TO_KCAL    = 4.184f;
    // Normalized Power window: 30 samples x 1 Hz (RTC-second gated).
    static constexpr uint8_t NP_WINDOW   = 30;

    // --- NP rolling window state --------------------------------------------
    // Ring of power^4 samples so the running mean can be updated in O(1);
    // zero-filled slots contribute 0^4 = 0, so the warm-up mean is correct.
    float  _npRing[NP_WINDOW] = { 0.0f };
    uint8_t _npIdx   = 0;
    uint8_t _npCount = 0;
    float  _npSum4   = 0.0f;   // sum of p^4 over the window
    bool   _hasNpSec = false;
    time_t _lastNpSec = -1;    // last RTC second a sample was pushed

    // One power sample per RTC second (1 Hz), regardless of main-loop cadence.
    void samplePower(float watts, time_t sec);
    void updateNp();
    // The Key-et-al per-second calorie rate for a live HR sample; the exact
    // same constants TCXLogger uses, so live and logged calories agree.
    float hrCaloriePerSec(float bpm, const FitnessProfile& p) const;
    uint8_t hrZoneFor(float bpm, const FitnessProfile& p) const;
    uint8_t powerZoneFor(float watts, const FitnessProfile& p) const;

    FitnessOutput _out{};
};

#endif /* FITNESSFUSION_H */
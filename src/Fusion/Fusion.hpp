#ifndef FUSION_H
#define FUSION_H

#include "AltitudeFusion.hpp"
#include "FitnessFusion.hpp"
#include "HAL/Measurements.hpp"

// =============================================================================
// FusionEngine -- the App layer's interpretation stage.
//
// Consumes the HAL's MeasurementFrame (raw, timestamped facts) and produces
// the derived channels: fused altitude/vario, selected speed, grade, and
// per-gate distance. This is where "what the world did" becomes "what it
// means": nothing here belongs in HAL, and nothing acquired belongs here.
//
// Every stage runs once per App tick, gated on the frame's per-channel
// sequence numbers, so each estimator sees each acquired sample exactly once
// with the interval it was actually sampled at (Sample::dtMs), regardless of
// how fast the main loop spins or how busy the I2C bus got.
// =============================================================================

struct DerivedChannels {
    // Altitude: fused while the barometer reports, otherwise the last GPS
    // altitude -- exactly the fallback getAltitude() used to perform.
    float altitudeM     = 0.0f;
    bool  altitudeValid = false;
    // Raw baro altitude computed from pressure and P0_local, BEFORE the
    // filter's bias trim. Diagnostic: compare against GpsAltitude to tell a
    // GPS-anchor error (both off together) from a filter-state error
    // (Altitude off alone).
    float baroAltitudeM      = 0.0f;
    bool  baroAltitudeValid  = false;
    // Vertical velocity from the fusion filter (was altVelocity()).
    float varioMs       = 0.0f;
    bool  varioValid    = false;
    // Speed: wheel revolutions x circumference when a wheel sensor lives,
    // otherwise GPS speed.
    float speedKmh      = 0.0f;
    // Grade: vario converted to km/h over speed, as a percentage.
    float gradePct      = 0.0f;
    // Per-gate distance increment (metres). Accumulation stays in
    // TelemetryDataProvider, exactly as before.
    float distanceDeltaM = 0.0f;
    // Accumulated ride distance (metres). Sum of distanceDeltaM since the
    // last FusionEngine::resetDistance() -- previously accumulated inside
    // TelemetryDataProvider::update, now lives with the delta's producer.
    float totalDistanceM = 0.0f;
    // Ambient temperature: barometer's own sensor when valid, otherwise the
    // RTC's. This fold used to live in HAL::update() -- it is an interpretation
    // (which source to trust), not an acquisition, so it belongs here.
    float temperatureC   = 0.0f;

    // --- New derived signals ---
    // Cadence in RPM (from crank sensor). 0 when no crank data available.
    float cadenceRpm    = 0.0f;
    bool  cadenceValid  = false;
    // Gear ratio: wheel RPM / crank RPM. Dimensionless, 0 when not computable.
    // Higher = harder gear. Guarded: requires both sensors live and cadence > min.
    float gearRatio     = 0.0f;
    bool  gearRatioValid = false;
    // Accumulated energy in kilojoules (integral of power over time).
    float energyKj      = 0.0f;
    // Acceleration in m/s^2 (derivative of speed). Positive = accelerating.
    float accelMs2      = 0.0f;
    // Estimated power in watts (physics-based from speed, grade, mass).
    // Used when no power meter is connected.
    float estPowerWatts = 0.0f;
    bool  estPowerValid = false;
    // Total accumulated ascent in metres (from positive vario).
    float totalAscentM  = 0.0f;
    // Total accumulated descent in metres (from negative vario, stored positive).
    float totalDescentM = 0.0f;
    // Coasting detection: true when speed is significant but power is negligible.
    bool  coasting      = false;
    // Fitness channels -- accumulated by FitnessFusion over the current ride
    // (reset exactly when distance resets, i.e. logging start).
    float caloriesKcal       = 0.0f;
    uint8_t hrZone           = 0;      // 0 = no HR / below zone 1; else 1..5
    uint8_t powerZone        = 0;      // 0 = no power / FTP unset; else 1..5
    float normalizedPowerW   = 0.0f;   // 30 s rolling NP (power meter)
    float intensityFactor    = 0.0f;   // NP / FTP
    float tss                = 0.0f;   // accumulated TSS
    // Seconds accumulated in each HR zone: index 0..4 == zone1..zone5.
    float timeInZoneSec[5]   = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    // 3 s rolling display power (meter when live, else physics estimate).
    float avg3sPowerW        = 0.0f;
    // Current power (meter or estimate) per kg of rider mass; 0 when neither
    // source is available or the rider mass is unset.
    float wattsPerKg         = 0.0f;
    // Vertical ascent rate (vario) in metres per hour; 0 when stationary.
    float vamMph             = 0.0f;
    // GNSS fix quality: 0 = Good, 1 = Fair, 2 = Poor (see GpsQuality thresholds).
    uint8_t gpsQuality       = 0;
    // Gear-shift detection (from gearRatio step changes; see FusionEngine).
    uint32_t shiftCount      = 0;      // detected shifts this ride (reset with distance)
    bool     gearShifted     = false;  // true on the tick a shift was confirmed
};

class FusionEngine {
public:
    // One call per App tick. wheelCircumferenceMm comes from the bike stats
    // model and profile is the per-tick snapshot of the rider's bio/bike
    // settings (mass, age, FTP, zone starts), so the estimators need no
    // knowledge of where they are stored.
    void update(const MeasurementFrame& f, uint16_t wheelCircumferenceMm,
                const FitnessProfile& profile);

    // Zero the accumulated ride distance. Called by App when logging starts
    // or the SD card state changes (previously TelemetryDataProvider::resetDistance).
    // Ride-scoped accumulators that share the distance's lifetime (energy,
    // calories, TSS, time-in-zone) reset here too, via FitnessFusion::reset().
    void resetDistance() {
        _totalDistanceM = 0.0f;
        _fitness.reset();
        _shiftCount = 0;
        _hasStableGear = false;
        _shiftConfirmTicks = 0;
    }

    const DerivedChannels& out() const { return _out; }

private:
    AltitudeFusion _alt;
    FitnessFusion _fitness;

    // Previous frame, for per-channel sequence-number gating.
    MeasurementFrame _prev{};
    bool _hasPrev = false;

    // DistanceTracker state -- moved verbatim from App::updateTelemetry()
    // (App's _lastLocation TinyGPSLocation copy and its validity flag).
    double _lastLat = 0.0, _lastLng = 0.0;
    bool   _hasLastPos = false;

    // Low-pass filter state for grade smoothing.
    // The vario (est_vel_z) is derived from IMU integration and can be noisy;
    // the raw grade (vario/speed) inherits that noise, producing the volatile
    // readings in the hundreds. A first-order EMA smooths the output without
    // disturbing the fusion filter's own state.
    float _filteredGradePct = 0.0f;
    bool  _hasGradeSample = false;

    // Grade filter alpha: lower = smoother but more lag, higher = more responsive.
    // 0.1 gives ~10-sample time constant, good for steady-state grade readings.
    static constexpr float GRADE_FILTER_ALPHA = 0.1f;

    // --- State for new derived signals ---
    // Energy accumulator state (integral of power over time).
    float _energyKj = 0.0f;
    // Previous speed for acceleration calculation.
    float _prevSpeedMs = 0.0f;
    bool  _hasPrevSpeed = false;
    // Total ascent/descent accumulators.
    float _totalAscentM = 0.0f;
    float _totalDescentM = 0.0f;
    // Accumulated ride distance (metres); zeroed by resetDistance().
    float _totalDistanceM = 0.0f;

    // --- Gear-shift detection state ---
    float   _stableGear     = 0.0f;   // gear ratio the tracker currently believes
    uint8_t _shiftConfirmTicks = 0;   // consecutive out-of-band samples
    uint32_t _shiftCount    = 0;      // confirmed shifts this ride
    bool    _hasStableGear  = false;

    // --- GNSS quality state ---
    uint8_t _gpsQuality     = 0;      // 0 Good, 1 Fair, 2 Poor
    uint8_t _gpsDegradeTicks = 0;     // consecutive samples worse than current

    // --- Constants for derived calculations ---
    // Estimated power physics model parameters.
    // Total mass (rider + bike) in kg comes from FitnessProfile::totalMassKg(),
    // so the physics estimate tracks the rider/bike settings on the SD card.
    // Rolling resistance coefficient (asphalt, typical tyres).
    static constexpr float EST_POWER_CRR = 0.005f;
    // Aerodynamic drag coefficient * frontal area (CdA), m^2.
    static constexpr float EST_POWER_CDA = 0.4f;
    // Air density at sea level, kg/m^3.
    static constexpr float EST_POWER_AIR_DENSITY = 1.225f;
    // Drivetrain efficiency loss factor (multiply result by this).
    static constexpr float EST_POWER_DRIVETRAIN_LOSS = 1.03f;
    // Minimum cadence for valid gear ratio calculation (RPM).
    static constexpr float GEAR_RATIO_MIN_CADENCE_RPM = 20.0f;
    // Coasting detection thresholds.
    static constexpr float COASTING_MIN_SPEED_KMH = 10.0f;
    static constexpr float COASTING_MAX_POWER_W = 10.0f;

    // Gear-shift detection: a shift is confirmed when the gear ratio moves
    // away from the tracked stable value by at least GEAR_SHIFT_DELTA for
    // GEAR_SHIFT_CONFIRM_TICKS consecutive valid samples (rejects momentary
    // cadence/speed noise), with cadence above the gear-ratio minimum.
    static constexpr float   GEAR_SHIFT_DELTA         = 0.15f;
    static constexpr uint8_t GEAR_SHIFT_CONFIRM_TICKS = 2;

    // GNSS quality: Poor when there is no fix, HDOP is very high or too few
    // satellites; Fair one band up. A demotion needs GPS_DEGRADE_TICKS
    // consecutive worse samples (hysteresis); promotion is immediate.
    static constexpr float   GPS_POOR_HDOP   = 2.5f;
    static constexpr float   GPS_FAIR_HDOP   = 1.5f;
    static constexpr uint8_t GPS_POOR_SATS   = 4;
    static constexpr uint8_t GPS_FAIR_SATS   = 6;
    static constexpr uint8_t GPS_DEGRADE_TICKS = 3;

    DerivedChannels _out{};

    void updateDistance(const MeasurementFrame& f);
};

#endif /* FUSION_H */
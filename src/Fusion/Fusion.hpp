#ifndef FUSION_H
#define FUSION_H

#include "AltitudeFusion.hpp"
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
};

class FusionEngine {
public:
    // One call per App tick. wheelCircumferenceMm comes from the bike stats
    // model, so the speed estimator needs no knowledge of where it is stored.
    void update(const MeasurementFrame& f, uint16_t wheelCircumferenceMm);

    const DerivedChannels& out() const { return _out; }

private:
    AltitudeFusion _alt;

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

    // --- Constants for derived calculations ---
    // Estimated power physics model parameters.
    // Total mass (rider + bike) in kg. Used for gravity and acceleration terms.
    static constexpr float EST_POWER_MASS_KG = 80.0f;
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

    DerivedChannels _out{};

    void updateDistance(const MeasurementFrame& f);
};

#endif /* FUSION_H */
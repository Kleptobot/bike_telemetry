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

    DerivedChannels _out{};

    void updateDistance(const MeasurementFrame& f);
};

#endif /* FUSION_H */
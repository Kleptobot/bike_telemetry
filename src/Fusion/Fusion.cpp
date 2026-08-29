#include "Fusion.hpp"
#include <cmath>

void FusionEngine::update(const MeasurementFrame& f, uint16_t wheelCircumferenceMm) {
    const bool firstTick = !_hasPrev;

    // --- Stage 1: barometer -> fused altitude --------------------------------
    // Was HAL's onDPS callback. Gated on the baro sample's sequence number,
    // which SensorSystem increments only on successful fetches, so a failed
    // bus transaction no longer re-feeds a stale pressure into the filter
    // the way the every-attempt callback did.
    if (firstTick || f.baroPressurePa.seq != _prev.baroPressurePa.seq) {
        if (f.baroPressurePa.valid) {
            const float baroAlt = _alt.computeAltitudeFromPressure(
                f.baroPressurePa.value, f.baroTempC.value);
            if (_alt.seaLevelPressureCalibrated()) {
                _alt.altitudeDPSUpdate(baroAlt);
            }
        }
    }

    // --- Stage 2: IMU integration --------------------------------------------
    // Was HAL's onIMU callback. The sign flip is the board's mounting
    // convention: the filter wants accZ positive-up and the LSM6DS3 frame
    // the HAL reads is positive-down.
    if (firstTick || f.imu.seq != _prev.imu.seq) {
        // dt is the interval stamped at acquisition (Sample::dtMs) rather
        // than a micros() diff taken at consumption time -- the same value,
        // but deterministic and host-testable. AltitudeFusion's dt clamp
        // (kept verbatim) rescales anything over 0.1 s down to 0.01 s, which
        // at the current 1 Hz IMU rate means integration runs at 1/100
        // scale; that is today's on-device behaviour, preserved on purpose.
        _alt.altitudeIMUUpdate(-1.0f * f.imu.value.f32_acc_z,
                               (float)f.imu.dtMs / 1000.0f);
    }

    // --- Stage 3: GPS correction on each new altitude commit ------------------
    // Was HAL's age-shrink detection; a sequence-number change is the same
    // event (AgeSeqTracker notes a commit exactly when age() shrinks).
    // The validity gate is load-bearing: before the first fix the frame
    // carries altitude 0.0 with seq unchanged, and the original trigger
    // (age() < previous age) could never fire in that state. Feeding 0.0
    // here anyway would bootstrap est_alt to 0 AND set altitudeValidLast,
    // so the first real fix would take the bias-trim branch instead of the
    // bootstrap branch -- altitude would sit near zero for thousands of
    // seconds while k_gps trimmed the bias.
    if (f.gpsAltM.valid &&
        (firstTick || f.gpsAltM.seq != _prev.gpsAltM.seq)) {
        _alt.altitudeGPSCorrect((float)f.gpsAltM.value);
    }

    // --- Stage 4: baro calibration from GPS (was HAL's 1 Hz HDOP block) -------
    // Runs on each new altitude commit instead of on a 1 Hz poll; the
    // calibrator's own 30 s cooldown and 5-sample stability history make the
    // cadence equivalent, with distinct fixes rather than repeated identical
    // reads filling the history. The baro-validity guard is new: the original
    // fed whatever pressure sat in dps_data -- 0 Pa before the first
    // successful fetch -- which could bootstrap P0 from garbage during the
    // first seconds after boot.
    if (firstTick || f.gpsAltM.seq != _prev.gpsAltM.seq) {
        if (f.baroPressurePa.valid && f.gpsAltM.valid &&
            f.gpsHdop.valid && f.gpsHdop.value < 2.0) {
            _alt.baroCalibrateFromGPS((float)f.gpsAltM.value,
                                      f.baroPressurePa.value,
                                      f.baroTempC.value);
        }
    }

    // --- Stage 5: altitude / vario outputs ------------------------------------
    // Exactly the fallback getAltitude() and altVelocity() used to perform.
    _out.varioMs    = _alt.rise();
    _out.varioValid = f.baroPressurePa.valid;
    if (f.baroPressurePa.valid) {
        _out.altitudeM     = _alt.altitude();
        _out.altitudeValid = true;
    } else {
        _out.altitudeM     = (float)f.gpsAltM.value;
        _out.altitudeValid = f.gpsAltM.valid;
    }

    // --- Stage 5b: temperature source selection --------------------------------
    // Was HAL's f32_temp fold: baro sensor when valid, RTC otherwise. An
    // interpretation (which source to trust), not an acquisition, so it lives
    // here, not in HAL.
    if (f.baroTempC.valid) {
        _out.temperatureC = f.baroTempC.value;
    } else if (f.rtcTempC.valid) {
        _out.temperatureC = (float)f.rtcTempC.value;
    } else {
        _out.temperatureC = 0.0f;
    }

    // --- Stage 6: speed selection (verbatim from App::updateTelemetry) --------
    _out.speedKmh = 0.0f;
    // Treat a zero circumference as "no wheel data" rather than computing a
    // speed of zero from it. Without this, an unconfigured circumference
    // silently suppressed the GPS fallback: wheelRPM.live is true whenever a
    // CSC sensor is connected, so the first branch was taken and produced 0.
    if (f.wheelRpmFiltered.live && wheelCircumferenceMm > 0) {
        _out.speedKmh = f.wheelRpmFiltered.value * wheelCircumferenceMm * 0.00006;
    } else if (f.gpsSpeedKmh.valid) {
        _out.speedKmh = (float)f.gpsSpeedKmh.value;
    }

    // --- Stage 7: grade (verbatim from App::updateTelemetry) -------------------
    _out.gradePct = 0.0f;
    if (_out.speedKmh > 0 && _out.varioValid) {
        //rise in m/s * 3.6 to convert to km/h, then divide by speed in km/h to get grade as a percentage
        _out.gradePct = _out.varioMs*3.6f*100.0f/_out.speedKmh;
    }

    // --- Stage 8: distance (verbatim, see updateDistance) ----------------------
    updateDistance(f);

    _prev = f;
    _hasPrev = true;
}

// Haversine formula, moved verbatim from App::updateTelemetry. State was
// App's _lastLocation (a TinyGPSLocation copy) and _lastSeconds (epoch
// seconds); the validity flag travels as _hasLastPos.
void FusionEngine::updateDistance(const MeasurementFrame& f) {
    _out.distanceDeltaM = 0.0f;

    // Gate on the RTC sample. The original fired when the RTC's epoch second
    // changed; the frame's sequence number fires per accepted RTC read
    // (950 ms period). GPS fixes arrive at 1 Hz and unchanged positions
    // contribute zero, so each fix-to-fix segment is still counted exactly
    // once either way.
    if (!_hasPrev || f.rtcNow.seq != _prev.rtcNow.seq) {
        if (f.gpsPos.valid && _hasLastPos) {
            double deg2rad = M_PI/180.0;
            double theta1 = _lastLat*deg2rad;
            double theta2 = f.gpsPos.lat*deg2rad;
            double phi1   = _lastLng*deg2rad;
            double phi2   = f.gpsPos.lng*deg2rad;

            double s1 = sin((theta2 - theta1)/2.0);
            s1 = s1*s1;
            double c1 = cos(theta1) * cos(theta2);
            double s2 = sin((phi2-phi1)/2.0);
            s2 = s2*s2;

            _out.distanceDeltaM = 2.0*6371000.0*asin(sqrt(s1+c1*s2)); //distance in m
        }
        _lastLat = f.gpsPos.lat;
        _lastLng = f.gpsPos.lng;
        _hasLastPos = f.gpsPos.valid;
    }
}
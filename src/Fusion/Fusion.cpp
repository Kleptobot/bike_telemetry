#include "Fusion.hpp"
#include <cmath>

void FusionEngine::update(const MeasurementFrame& f, uint16_t wheelCircumferenceMm,
                          const FitnessProfile& profile) {
    const bool firstTick = !_hasPrev;

    // Observed RTC interval for this tick, sanity-clamped exactly as each
    // consumer used to do inline: 0 when there is no usable interval.
    float dtSec = 0.0f;
    if (_hasPrev) {
        const float raw = (float)(f.rtcNow.tsMs - _prev.rtcNow.tsMs) / 1000.0f;
        if (raw > 0.0f && raw < 5.0f) dtSec = raw;
    }

    // --- Stage 1: barometer -> fused altitude --------------------------------
    // Was HAL's onDPS callback. Gated on the baro sample's sequence number,
    // which SensorSystem increments only on successful fetches, so a failed
    // bus transaction no longer re-feeds a stale pressure into the filter
    // the way the every-attempt callback did.
    if (firstTick || f.baroPressurePa.seq != _prev.baroPressurePa.seq) {
        if (f.baroPressurePa.valid) {
            const float baroAlt = _alt.computeAltitudeFromPressure(
                f.baroPressurePa.value, f.baroTempC.value);
            // Surface the un-filtered baro altitude for diagnostics: it is
            // the input the whole filter anchors to, and the three-way
            // comparison with GpsAlt and the fused Altitude shows which
            // anchor (P0_local, baro_bias_z or est_vel_z) is off.
            // Before P0 is GPS-calibrated this is standard-atmosphere.
            _out.baroAltitudeM = baroAlt;
            _out.baroAltitudeValid = _alt.seaLevelPressureCalibrated();
            if (_alt.seaLevelPressureCalibrated()) {
                _alt.altitudeDPSUpdate(baroAlt);
            }
        } else {
            _out.baroAltitudeValid = false;
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

    // --- Stage 7: grade --------------------------------------------------------
    // The raw grade is vario (m/s -> km/h) / speed (km/h) * 100 for percent.
    // vario comes from IMU integration and is inherently noisy, so the raw
    // grade can swing wildly (readings in the hundreds). Apply a first-order
    // low-pass filter to smooth the output without disturbing the fusion
    // filter's internal state.
    //
    // Minimum speed for meaningful grade calculation (km/h). Below this,
    // raw grade is forced to 0 -- dividing by near-zero speed would produce
    // absurd values (e.g. -500% at 0.1 km/h). The filter then decays toward 0.
    static constexpr float GRADE_MIN_SPEED_KMH = 5.0f;

    float rawGradePct = 0.0f;
    if (_out.speedKmh >= GRADE_MIN_SPEED_KMH && _out.varioValid) {
        rawGradePct = _out.varioMs * 3.6f * 100.0f / _out.speedKmh;
    }

    if (!_hasGradeSample) {
        // Initialize the filter with the first sample to avoid a long ramp.
        _filteredGradePct = rawGradePct;
        _hasGradeSample = true;
    } else {
        // Exponential moving average: y[n] = y[n-1] + alpha * (x[n] - y[n-1])
        _filteredGradePct += GRADE_FILTER_ALPHA * (rawGradePct - _filteredGradePct);
    }

    _out.gradePct = _filteredGradePct;

    // --- Stage 8: cadence ------------------------------------------------------
    // Derive cadence in RPM from the crank EventCount. The HAL already
    // pre-computes cadenceRpmFiltered from the CSC parser; we just surface it.
    _out.cadenceValid = f.cadenceRpmFiltered.live;
    _out.cadenceRpm = f.cadenceRpmFiltered.live ? f.cadenceRpmFiltered.value : 0.0f;

    // --- Stage 9: gear ratio ---------------------------------------------------
    // Gear ratio = wheel RPM / crank RPM. Both must be live and cadence must
    // be above minimum (avoid division by near-zero when coasting/stopped).
    _out.gearRatioValid = false;
    _out.gearRatio = 0.0f;
    if (f.wheelRpmFiltered.live && f.cadenceRpmFiltered.live &&
        f.cadenceRpmFiltered.value >= GEAR_RATIO_MIN_CADENCE_RPM) {
        _out.gearRatio = f.wheelRpmFiltered.value / f.cadenceRpmFiltered.value;
        _out.gearRatioValid = true;
    }

    // --- Stage 10: energy (integral of power) ----------------------------------
    // Accumulate energy from power meter data. dt from the RTC tick cadence.
    // E(kJ) = P(W) * dt(s) / 1000
    if (f.powerWatts.live && dtSec > 0.0f) {
        _energyKj += f.powerWatts.value * dtSec / 1000.0f;
    }
    _out.energyKj = _energyKj;

    // --- Stage 11: acceleration (derivative of speed) --------------------------
    // a = dv/dt. Use the selected speed and RTC interval.
    _out.accelMs2 = 0.0f;
    if (_hasPrevSpeed && dtSec > 0.0f) {
        float speedMs = _out.speedKmh / 3.6f;
        _out.accelMs2 = (speedMs - _prevSpeedMs) / dtSec;
    }
    _prevSpeedMs = _out.speedKmh / 3.6f;
    _hasPrevSpeed = true;

    // --- Stage 12: estimated power (physics model) -----------------------------
    // Estimate power from speed, grade, and total (rider + bike) mass when no
    // power meter is connected.
    // P = (F_roll + F_gravity + F_air + F_accel) * v / efficiency
    const float totalMassKg = profile.totalMassKg();
    _out.estPowerValid = false;
    _out.estPowerWatts = 0.0f;
    if (_out.speedKmh >= GRADE_MIN_SPEED_KMH && _out.varioValid) {
        float v = _out.speedKmh / 3.6f;  // speed in m/s
        float gradeFrac = _out.gradePct / 100.0f;  // grade as fraction

        // Rolling resistance: F_rr = CRR * m * g * cos(theta) ≈ CRR * m * g (small angles)
        float f_roll = EST_POWER_CRR * totalMassKg * STANDARD_GRAVITY;

        // Gravity: F_g = m * g * sin(theta) ≈ m * g * grade (small angles)
        float f_gravity = totalMassKg * STANDARD_GRAVITY * gradeFrac;

        // Aerodynamic drag: F_ad = 0.5 * rho * CdA * v^2
        float f_air = 0.5f * EST_POWER_AIR_DENSITY * EST_POWER_CDA * v * v;

        // Acceleration: F_a = m * a
        float f_accel = totalMassKg * _out.accelMs2;

        // Total force * velocity = power, with drivetrain loss
        float power = (f_roll + f_gravity + f_air + f_accel) * v * EST_POWER_DRIVETRAIN_LOSS;

        // Only positive power is meaningful (can't estimate regenerative braking)
        _out.estPowerWatts = max(0.0f, power);
        _out.estPowerValid = true;
    }

    // --- Stage 13: total ascent / descent --------------------------------------
    // Accumulate from vario (vertical velocity) when valid.
    if (_out.varioValid && dtSec > 0.0f) {
        float deltaAlt = _out.varioMs * dtSec;  // metres changed this tick
        if (deltaAlt > 0.0f) {
            _totalAscentM += deltaAlt;
        } else {
            _totalDescentM += -deltaAlt;  // store as positive value
        }
    }
    _out.totalAscentM = _totalAscentM;
    _out.totalDescentM = _totalDescentM;

    // --- Stage 14: coasting detection ------------------------------------------
    // Coasting = moving but not pedalling (low power, adequate speed).
    _out.coasting = (_out.speedKmh >= COASTING_MIN_SPEED_KMH) &&
                     (!f.powerWatts.live || f.powerWatts.value <= COASTING_MAX_POWER_W);

    // --- Stage 14b: fitness channels (calories, zones, NP/IF/TSS) --------------
    // Runs after the power stages so it can pick between the power meter and
    // the physics estimate. dtSec is shared with stages 10-13 above.
    {
        const float powerWatts = f.powerWatts.live ? f.powerWatts.value : 0.0f;
        _fitness.update(f, profile, dtSec, powerWatts, _out.estPowerWatts);
        const FitnessOutput& fit = _fitness.out();
        _out.caloriesKcal     = fit.caloriesKcal;
        _out.hrZone           = fit.hrZone;
        _out.powerZone        = fit.powerZone;
        _out.normalizedPowerW = fit.normalizedPowerW;
        _out.intensityFactor  = fit.intensityFactor;
        _out.tss              = fit.tss;
        for (int i = 0; i < 5; i++) _out.timeInZoneSec[i] = fit.timeInZoneSec[i];
    }

    // --- Stage 15: distance (verbatim, see updateDistance) ---------------------
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
        _totalDistanceM += _out.distanceDeltaM;
        _out.totalDistanceM = _totalDistanceM;
        _lastLat = f.gpsPos.lat;
        _lastLng = f.gpsPos.lng;
        _hasLastPos = f.gpsPos.valid;
    }
}
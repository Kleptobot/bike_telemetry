#include "Fusion/FitnessFusion.hpp"
#include <math.h>

// ---------------------------------------------------------------------------
// Zone classification ------------------------------------------------------
// ---------------------------------------------------------------------------

uint8_t FitnessFusion::hrZoneFor(float bpm, const FitnessProfile& p) const {
    if (bpm <= 0.0f) return 0;
    if (bpm >= p.zoneStartsBpm[4]) return 5;
    if (bpm >= p.zoneStartsBpm[3]) return 4;
    if (bpm >= p.zoneStartsBpm[2]) return 3;
    if (bpm >= p.zoneStartsBpm[1]) return 2;
    if (bpm >= p.zoneStartsBpm[0]) return 1;
    return 0;
}

uint8_t FitnessFusion::powerZoneFor(float watts, const FitnessProfile& p) const {
    // Standard seven-zone-by-power collapses to five unless FTP is set.
    if (watts <= 0.0f || p.ftpWatts == 0) return 0;
    const float frac = watts / (float)p.ftpWatts;
    if (frac >= 1.05f) return 5;   // > 105 % FTP: VO2 / anaerobic
    if (frac >= 0.90f) return 4;   // 90-105 %: threshold
    if (frac >= 0.75f) return 3;   // 75-90 %: tempo
    if (frac >= 0.55f) return 2;   // 55-75 %: endurance
    return 1;                      // < 55 %: recovery
}

// ---------------------------------------------------------------------------
// Calorie rate ----------------------------------------------------------------
// ---------------------------------------------------------------------------

float FitnessFusion::hrCaloriePerSec(float bpm, const FitnessProfile& p) const {
    // Key et al. (1989) male/female estimates, expressed as kcal per second.
    // The constants (including the sign of the mass term) are taken verbatim
    // from TCXLogger::writeLapHeader so the live tile integrates to exactly
    // the same total the TCX lap header reports. "Other" averages the two,
    // mirroring the logger's midpoint behaviour.
    const float age  = p.ageYears;
    const float mass = p.riderMassKg;

    const float f = (age * 0.074f - mass * 0.1265672342f + bpm * 0.4472f - 20.4022f) / 251.1f;
    const float m = (age * 0.2017f - mass * 0.1992094632f + bpm * 0.6309f - 55.0969f) / 251.1f;

    switch (p.caloricProfile) {
        case CaloricProfile::Female: return f;
        case CaloricProfile::Male:   return m;
        case CaloricProfile::Other:
        default:                     return (m + f) * 0.5f;
    }
}

// ---------------------------------------------------------------------------
// NP rolling window -----------------------------------------------------------
// ---------------------------------------------------------------------------

void FitnessFusion::samplePower(float watts, time_t sec) {
    // Gate to one sample per RTC second so the 30-slot ring really is 30 s
    // no matter how fast the main loop spins or how busy the I2C bus got.
    if (_hasNpSec && sec == _lastNpSec) return;
    _lastNpSec = sec;
    _hasNpSec  = true;

    const float p4 = watts * watts * watts * watts;
    _npSum4 += p4 - _npRing[_npIdx];   // slot starts 0 warm-up => subtracts nothing
    _npRing[_npIdx] = p4;
    _npIdx = (_npIdx + 1) % NP_WINDOW;
    if (_npCount < NP_WINDOW) _npCount++;
}

void FitnessFusion::updateNp() {
    if (_npCount == 0) {
        _out.normalizedPowerW = 0.0f;
        return;
    }
    // NP = (mean(p^4))^(1/4), the standard weighted (rolling-30) plateau.
    _out.normalizedPowerW = powf(_npSum4 / (float)_npCount, 0.25f);
}

// ---------------------------------------------------------------------------
// Update / reset ----------------------------------------------------------------
// ---------------------------------------------------------------------------

void FitnessFusion::update(const MeasurementFrame& f, const FitnessProfile& profile,
                           float dtSec, float powerWatts, float estPowerWatts) {
    // No usable tick interval this round (first tick, RTC stall, or a
    // > 5 s gap): nothing can be integrated or classified meaningfully.
    if (!(dtSec > 0.0f && dtSec < 5.0f)) return;

    const float bpm = f.heartRateBpm.live ? f.heartRateBpm.value : 0.0f;

    // --- HR zone and time in zone ------------------------------------------
    _out.hrZone = hrZoneFor(bpm, profile);
    if (_out.hrZone >= 1 && _out.hrZone <= HR_ZONES) {
        _out.timeInZoneSec[_out.hrZone - 1] += dtSec;
    }

    // --- Calories: power meter > physics estimate > HR -----------------------
    // Mechanical kJ -> dietary kcal at ~24 % gross efficiency:
    //   kcal = kJ / (EFFICIENCY * kJ_per_kcal)   (180 W for an hour ~ 645 kcal)
    float kcal = 0.0f;
    if (powerWatts > 0.0f) {
        kcal = powerWatts * dtSec / 1000.0f / (EFFICIENCY * KJ_TO_KCAL);
    } else if (estPowerWatts > 0.0f) {
        kcal = estPowerWatts * dtSec / 1000.0f / (EFFICIENCY * KJ_TO_KCAL);
    } else if (bpm > 0.0f) {
        kcal = hrCaloriePerSec(bpm, profile) * dtSec;
    }
    if (kcal > 0.0f) {
        _out.caloriesKcal += kcal;
    }

    // --- Normalized Power + power zone (power meter only) --------------------
    samplePower(powerWatts, f.rtcNow.value);
    updateNp();
    _out.powerZone = powerZoneFor(powerWatts, profile);

    // --- Intensity Factor and TSS ---------------------------------------------
    if (profile.ftpWatts > 0 && _out.normalizedPowerW > 0.0f) {
        _out.intensityFactor = _out.normalizedPowerW / (float)profile.ftpWatts;
        // TSS = IF^2 x hours x 100; integrate the same dtSec as the calories.
        _out.tss += _out.intensityFactor * _out.intensityFactor
                  * (dtSec / 3600.0f) * 100.0f;
    } else {
        _out.intensityFactor = 0.0f;
    }
}

void FitnessFusion::reset() {
    _out = FitnessOutput{};
    _npCount = 0;
    _npIdx   = 0;
    _npSum4  = 0.0f;
    for (uint8_t i = 0; i < NP_WINDOW; i++) _npRing[i] = 0.0f;
    _hasNpSec = false;
    _lastNpSec = -1;
}
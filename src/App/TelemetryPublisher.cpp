#include "TelemetryPublisher.hpp"

#include <Arduino.h>
#include <time.h>

#include "../HAL/HAL.hpp"

void TelemetryPublisher::begin(DataModel& model) {
    _model = &model;
    _prevGpsPosValid = false;
}

void TelemetryPublisher::resetDistance() {
    _fusion.resetDistance();
}

void TelemetryPublisher::update() {
    if (!_model) return;

    const MeasurementFrame& frame = HAL::inst().measurements();
    const auto& bio = _model->bio().get();
    const auto& bike = _model->bike().get();

    FitnessProfile profile;
    profile.riderMassKg = (float)bio.mass;
    profile.bikeMassKg = bike.mass / 10.0f;
    profile.ageYears = timeDuration(_model->time().get() - bio.birthday).years();
    profile.caloricProfile = bio.caloricProfile;
    profile.zoneStartsBpm[0] = (float)bio.zone1Start;
    profile.zoneStartsBpm[1] = (float)bio.zone2Start;
    profile.zoneStartsBpm[2] = (float)bio.zone3Start;
    profile.zoneStartsBpm[3] = (float)bio.zone4Start;
    profile.zoneStartsBpm[4] = (float)bio.zone5Start;
    profile.ftpWatts = bio.ftpWatts;

    _fusion.update(frame, bike.wheelCircumference, profile);
    const DerivedChannels& d = _fusion.out();

    DataBus& bus = _model->bus();
    bus.publish(Topic::FusedAltitude, d.altitudeM);
    bus.publish(Topic::BaroAlt, d.baroAltitudeM);
    bus.publish(Topic::Vario, d.varioMs);
    bus.publish(Topic::EstimatedSpeed, d.speedKmh);
    bus.publish(Topic::WheelSpeed, d.wheelSpeedKmh);
    bus.publish(Topic::SmoothedGrade, d.gradePct);
    bus.publish(Topic::Cadence, d.cadenceRpm);
    bus.publish(Topic::WheelRpm, d.wheelRpm);
    bus.publish(Topic::WheelRevs, frame.wheelRevs.counter);
    bus.publish(Topic::WheelRevsValid, d.wheelRevsValid);
    bus.publish(Topic::CrankRevs, frame.crankRevs.counter);
    bus.publish(Topic::CrankRevsValid, d.crankRevsValid);
    bus.publish(Topic::GearRatio, d.gearRatio);
    bus.publish(Topic::Energy, d.energyKj);
    bus.publish(Topic::Acceleration, d.accelMs2);
    bus.publish(Topic::EstimatedPower, d.estPowerWatts);
    bus.publish(Topic::TotalAscent, d.totalAscentM);
    bus.publish(Topic::TotalDescent, d.totalDescentM);
    bus.publish(Topic::Coasting, d.coasting);
    bus.publish(Topic::Calories, d.caloriesKcal);
    bus.publish(Topic::NormalizedPower, d.normalizedPowerW);
    bus.publish(Topic::IntensityFactor, d.intensityFactor);
    bus.publish(Topic::Tss, d.tss);
    bus.publish(Topic::HrZone, d.hrZone);
    bus.publish(Topic::PowerZone, d.powerZone);
    bus.publish(Topic::Avg3sPower, d.avg3sPowerW);
    bus.publish(Topic::WattsPerKg, d.wattsPerKg);
    bus.publish(Topic::Vam, d.vamMph);
    bus.publish(Topic::GpsQuality, d.gpsQuality);
    bus.publish(Topic::ShiftCount, d.shiftCount);
    bus.publish(Topic::GearShifted, d.gearShifted);

    location_data loc;
    loc.valid = frame.gpsPos.valid;
    loc.longitude = frame.gpsPos.lng;
    loc.latitude = frame.gpsPos.lat;

    bus.publish(Topic::GpsValid, frame.gpsPos.valid);
    bus.publish(Topic::Location, loc);
    bus.publish(Topic::DistanceDeltaM, d.distanceDeltaM);
    bus.publish(Topic::TotalDistanceM, d.totalDistanceM);
    bus.publish(Topic::GpsDistanceDeltaM, d.gpsDistanceDeltaM);
    bus.publish(Topic::GpsTotalDistanceM, d.gpsTotalDistanceM);
    bus.publish(Topic::WheelDistanceDeltaM, d.wheelDistanceDeltaM);
    bus.publish(Topic::WheelTotalDistanceM, d.wheelTotalDistanceM);
    bus.publish(Topic::Battery, frame.batteryPct);

    bus.publish(Topic::HeartRate,
                frame.heartRateBpm.live ? frame.heartRateBpm.value : 0.0f);
    bus.publish(Topic::PowerMeter,
                frame.powerWatts.live ? frame.powerWatts.value : 0.0f);
    bus.publish(Topic::Temperature, d.temperatureC);

    bus.publish(Topic::GpsSpeed,
                frame.gpsSpeedKmh.valid ? (float)frame.gpsSpeedKmh.value : 0.0f);
    bus.publish(Topic::GpsAltitude,
                frame.gpsAltM.valid ? (float)frame.gpsAltM.value : 0.0f);
    bus.publish(Topic::GpsCourse,
                frame.gpsCourseDeg.valid ? (float)frame.gpsCourseDeg.value : 0.0f);
    bus.publish(Topic::GpsSats, frame.gpsSatsUsed.value);
    bus.publish(Topic::GpsHdop,
                frame.gpsHdop.valid ? (float)frame.gpsHdop.value : 0.0f);
    bus.publish(Topic::BaroPressurePa,
                frame.baroPressurePa.valid ? frame.baroPressurePa.value : 0.0f);
    bus.publish(Topic::BaroPressureValid, frame.baroPressurePa.valid);
    bus.publish(Topic::BaroTemperatureC,
                frame.baroTempC.valid ? frame.baroTempC.value : 0.0f);
    bus.publish(Topic::BaroTemperatureValid, frame.baroTempC.valid);
    bus.publish(Topic::BaroSeaLevelPressurePa, d.baroSeaLevelPressurePa);
    bus.publish(Topic::BaroPressureCalibrated, d.baroPressureCalibrated);
    bus.publish(Topic::ImuSampleIntervalMs,
                frame.imu.valid ? (float)frame.imu.dtMs : 0.0f);
    bus.publish(Topic::ImuVerticalAcceleration, d.imuVerticalAccelerationMs2);
    bus.publish(Topic::ImuVerticalVelocity, d.imuVerticalVelocityMs);
    bus.publish(Topic::TorqueNm,
                frame.torqueNm.live ? frame.torqueNm.value : 0.0f);
    bus.publish(Topic::PedalBalance,
                frame.pedalBalancePct.live ? frame.pedalBalancePct.value : 0.0f);
    bus.publish(Topic::BatteryVolts, frame.vbatVolts.value);

    const imu_data& imu = frame.imu.value;
    bus.publish(Topic::ImuValid, frame.imu.valid);
    bus.publish(Topic::ImuAccelX, frame.imu.valid ? imu.f32_acc_x : 0.0f);
    bus.publish(Topic::ImuAccelY, frame.imu.valid ? imu.f32_acc_y : 0.0f);
    bus.publish(Topic::ImuAccelZ, frame.imu.valid ? imu.f32_acc_z : 0.0f);
    bus.publish(Topic::ImuGyroX, frame.imu.valid ? imu.f32_gyro_x : 0.0f);
    bus.publish(Topic::ImuGyroY, frame.imu.valid ? imu.f32_gyro_y : 0.0f);
    bus.publish(Topic::ImuGyroZ, frame.imu.valid ? imu.f32_gyro_z : 0.0f);

    if (frame.gpsPos.valid) {
        _model->gpsTrack().append(frame.gpsPos.lat, frame.gpsPos.lng, millis());
    }

    const int utcOffset = _model->time().get().offset();
    const bool newFix = frame.gpsPos.valid && !_prevGpsPosValid;
    if (newFix && frame.gpsUtcTimeHMS.valid && frame.gpsUtcDateYMD.valid) {
        const uint32_t hms = frame.gpsUtcTimeHMS.value;
        const uint32_t ymd = frame.gpsUtcDateYMD.value;
        struct tm gpsNow;
        gpsNow.tm_year = (ymd / 10000) - 1900;
        gpsNow.tm_mon = ((ymd / 100) % 100) - 1;
        gpsNow.tm_mday = ymd % 100;
        gpsNow.tm_hour = hms / 10000;
        gpsNow.tm_min = (hms / 100) % 100;
        gpsNow.tm_sec = hms % 100;
        gpsNow.tm_isdst = 0;

        time_t gpsEpoch = mktime(&gpsNow);
        time_t rtcEpoch = frame.rtcNow.value;
        time_t diff = difftime(gpsEpoch, rtcEpoch);
        if (diff < -30 || diff > 30) {
            HAL::inst().setTime(gpsNow);
        }
    }
    _prevGpsPosValid = frame.gpsPos.valid;

    _model->time().update({frame.rtcNow.value, utcOffset});
}

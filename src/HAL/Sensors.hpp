#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Dps3xx.h>
#include <LSM6DS3.h>
#include <functional>
#include "SensorData.hpp"
#if DS3231
    #include <RTClib.h>
#else
    #include <PCF85063A.h>
    #include <time.h>
#endif

#define LSM6DS3_ADDR 0x6A //I2C device address 0x6A
#define BAT_HIGH_CHARGE 22  // HIGH for 50mA, LOW for 100mA
#define BAT_CHARGE_STATE 23 // LOW for charging, HIGH not charging

// Volts per ADC least-significant bit, at the default 10-bit resolution and
// 3.6 V reference. (The name says MV/LBS; it is neither millivolts nor LBS.
// Left as-is to avoid churn -- see the cleanup PR.)
#define VBAT_MV_PER_LBS (0.003395996F)

// Usable terminal-voltage range of the LiPo cell, used to scale the reported
// battery percentage. 3.30 V is a conservative empty point that leaves margin
// above the protection cutoff.
#define BAT_FULL_V  (4.20F)
#define BAT_EMPTY_V (3.30F)

#define DS3231 false

class SensorSystem {
public:
    SensorSystem() {
        _myIMU = new LSM6DS3(I2C_MODE, LSM6DS3_ADDR); 
        _dps = Dps3xx();
    }
    void init_low();
    void init();
    bool update(bool i2cBusy);

    #if DS3231
        RTC_DS3231* RTC() {return &_rtc;}
    #else
        const PCF85063A & RTC() {return _rtc;}
    #endif
    
    time_t now() const {return _now;}
    int16_t batt() const {return _nBattPercentage;}
    imu_data imu() const {return _imu;}
    dps_data dps() const {return dps_dat;}
    void setTime(struct tm date);

    using DPSCallback = std::function<void(dps_data dps)>;
    using IMUCallback = std::function<void(data_record f32_acc_z)>;

    void onDPS(DPSCallback cb) { dpsCallback = cb; }
    void onIMU(IMUCallback cb) { imuCallback = cb; }

    // --- Measurement-frame stamping (see HAL/Measurements.hpp) --------------
    // Timestamps reuse the existing per-block read times; sequence numbers
    // count accepted samples; observed intervals are the time between
    // consecutive accepted samples (0 = not yet known). Pure bookkeeping:
    // no existing behaviour reads or changes because of these.
    uint32_t imuTsMs()  const { return lastIMUTime; }
    uint16_t imuSeq()   const { return _imuSeq; }
    uint16_t imuDtMs()  const { return _imuDtMs; }

    uint32_t dpsTsMs()  const { return _dpsOkTime; }   // last SUCCESSFUL fetch; lastDSPTime tracks attempts
    uint16_t dpsSeq()   const { return _dpsSeq; }
    uint16_t dpsDtMs()  const { return _dpsDtMs; }

    uint32_t rtcTsMs()  const { return lastRTCTime; }
    uint16_t rtcSeq()   const { return _rtcSeq; }
    uint16_t rtcDtMs()  const { return _rtcDtMs; }

    uint32_t battTsMs() const { return lastBATTime; }
    uint16_t battSeq()  const { return _batSeq; }
    uint16_t battDtMs() const { return _batDtMs; }

    // Promoted from a discarded local in update(): the gauge percentage is
    // derived from this voltage, which is the actual measurement.
    float vbatVolts() const { return _vBatVolts; }
    bool  charging()  const { return _charging; }

private:
    static const uint16_t BAT_Read_Period = 29999;
    // Step 5 ramp increment 1 (was 1000). Judged by the [i2c] summary:
    // bp must stay ~0, err flat, and the vario/grade tiles sane -- the
    // fusion dt-clamp makes the vario respond ~5x faster per wall-second
    // at this cadence than at 1 Hz.
    static const uint16_t IMU_Read_Period = 100;
    static const uint16_t DPS_Read_Period = 1000;
    static const uint16_t RTC_Read_Period = 950;

    LSM6DS3* _myIMU;
    #if DS3231
        RTC_DS3231 _rtc;
    #else
        PCF85063A _rtc;
    #endif

    Dps3xx _dps;
    
    uint32_t lastDSPTime = 0, lastIMUTime = 0, lastRTCTime = 0, lastBATTime = 0;
    imu_data _imu;
    dps_data dps_dat;
    bool _dpsValid = false;

    float _f32_RTC_Temp;
    struct tm  _newDate;
    time_t _now;
    int16_t _nBattPercentage;
    bool _setTime = false;

    DPSCallback dpsCallback;
    IMUCallback imuCallback;

    // Measurement-frame stamping state (see HAL/Measurements.hpp)
    uint16_t _imuSeq = 0, _dpsSeq = 0, _rtcSeq = 0, _batSeq = 0;
    uint16_t _imuDtMs = 0, _dpsDtMs = 0, _rtcDtMs = 0, _batDtMs = 0;
    uint32_t _dpsOkTime = 0;    // millis() of last successful pressure/temperature fetch
    float    _vBatVolts = 0.0f;
    bool     _charging = false;

};

#endif /* SENSORS_H */
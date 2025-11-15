#pragma once
#include <Arduino.h>
#include <RTClib.h>
#include <Dps3xx.h>
#include <LSM6DS3.h>
#include "SensorData.hpp"

#define LSM6DS3_ADDR 0x6A //I2C device address 0x6A
#define BAT_HIGH_CHARGE 22  // HIGH for 50mA, LOW for 100mA
#define BAT_CHARGE_STATE 23 // LOW for charging, HIGH not charging
#define VBAT_MV_PER_LBS (0.003395996F)

class SensorSystem {
public:
    SensorSystem() {
        _myIMU = new LSM6DS3(I2C_MODE, LSM6DS3_ADDR); 
        _dps = Dps3xx();
    }
    void init_low();
    void init();
    bool update(bool i2cBusy);

    const RTC_DS3231& RTC() {return _rtc;}
    DateTime now() const {return _now;}
    int16_t batt() const {return _nBattPercentage;}
    imu_data imu() const {return _imu;}
    dps_data dps() const {return dps_dat;}
    void setTime(DateTime date);

private:
    static const uint16_t BAT_Read_Period = 30001;
    static const uint16_t IMU_Read_Period = 73;
    static const uint16_t DPS_Read_Period = 101;
    static const uint16_t RTC_Read_Period = 99;

    LSM6DS3* _myIMU;
    RTC_DS3231 _rtc;
    Dps3xx _dps;
    
    uint32_t lastDSPTime = 0, lastIMUTime = 0, lastRTCTime = 0, lastBATTime = 0;
    imu_data _imu;
    dps_data dps_dat;

    float _f32_RTC_Temp;
    DateTime _now, _newDate;
    int16_t _nBattPercentage;
    bool _setTime = false;

};
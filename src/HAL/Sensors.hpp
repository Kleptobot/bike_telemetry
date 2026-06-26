#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <RTClib.h>
#include <Dps3xx.h>
#include <LSM6DS3.h>
#include "SensorData.hpp"

#define LSM6DS3_ADDR 0x6A //I2C device address 0x6A
#define BAT_HIGH_CHARGE 22  // HIGH for 50mA, LOW for 100mA
#define BAT_CHARGE_STATE 23 // LOW for charging, HIGH not charging
#define VBAT_MV_PER_LBS (0.003395996F)

#define DS3231 true

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
        const RTC_DS3231& RTC() {return _rtc;}
    #else
        const RTC_PCF8563& RTC() {return _rtc;}
    #endif
    DateTime now() const {return _now;}
    int16_t batt() const {return _nBattPercentage;}
    imu_data imu() const {return _imu;}
    dps_data dps() const {return dps_dat;}
    void setTime(DateTime date);

private:
    static const uint16_t BAT_Read_Period = 29999;
    static const uint16_t IMU_Read_Period = 1999;
    static const uint16_t DPS_Read_Period = 4999;
    static const uint16_t RTC_Read_Period = 450;

    LSM6DS3* _myIMU;
    #if DS3231
        RTC_DS3231 _rtc;
    #else
        RTC_PCF8563 _rtc;
    #endif

    Dps3xx _dps;
    
    uint32_t lastDSPTime = 0, lastIMUTime = 0, lastRTCTime = 0, lastBATTime = 0;
    imu_data _imu;
    dps_data dps_dat;
    bool _dpsValid = false;

    float _f32_RTC_Temp;
    DateTime _now, _newDate;
    int16_t _nBattPercentage;
    bool _setTime = false;

};

#endif /* SENSORS_H */
#include "Sensors.hpp"

void SensorSystem::init_low() {
    if (!_rtc.begin()) {
        Serial.println("Couldn't find RTC");
    } else {
        Serial.println("RTC initialised");
    }
    pinMode(VBAT_ENABLE, OUTPUT);
    pinMode(BAT_CHARGE_STATE, INPUT);

    digitalWrite(BAT_HIGH_CHARGE, HIGH); // charge with 50mA
}

void SensorSystem::init() {
    if (_myIMU->begin() != 0) {
        Serial.println("IMU init error");
    } else {
        Serial.println("IMU initialised");
    }
    
    _dps.begin(Wire);
    int16_t temp_mr = 2;
    int16_t temp_osr = 2;
    int16_t prs_mr = 2;
    int16_t prs_osr = 2;
    _dps.startMeasureBothCont(temp_mr, temp_osr, prs_mr, prs_osr);
}

bool SensorSystem::update(bool i2cBusy) {
    
    int16_t ret;
    uint8_t pressureCount = 20;
    float pressure[pressureCount];
    uint8_t temperatureCount = 20;
    float temperature[temperatureCount];
    bool update = false;

    if ((millis() - lastRTCTime > RTC_Read_Period) && !i2cBusy) {
        if (_setTime) {
            _setTime = false;
            _rtc.adjust(_newDate);
        }
        _f32_RTC_Temp = _rtc.getTemperature();
        dps_dat.f32_RTC_Temp = _f32_RTC_Temp;
        _now = _rtc.now();

        lastRTCTime = millis();
        update = true;
    } else if ((millis() - lastDSPTime > DPS_Read_Period) && !i2cBusy) {

        ret = _dps.getContResults(temperature, temperatureCount, pressure, pressureCount);
        //Dps3xxPressureSensor.measureTempOnce(f32_DSP_Temp, 7);
        //Dps3xxPressureSensor.measurePressureOnce(f32_DSP_Pa, 7);
        if (ret != 0)
        {
            // Serial.print("FAIL! ret = ");
        }
        else
        {
            dps_dat.f32_DSP_Temp=0;
            for (int16_t i = 0; i < temperatureCount; i++)
            {
            dps_dat.f32_DSP_Temp+=temperature[i];
            }
            dps_dat.f32_DSP_Temp = dps_dat.f32_DSP_Temp/(float)temperatureCount;

            dps_dat.f32_DSP_Pa=0;
            for (int16_t i = 0; i < pressureCount; i++)
            {
            dps_dat.f32_DSP_Pa+=pressure[i];
            }
            dps_dat.f32_DSP_Pa = dps_dat.f32_DSP_Pa/(float)pressureCount;

            //estimate altitude from pressure and temperature
            float Tb = 273.15+dps_dat.f32_DSP_Temp;
            float P_Pb = pow(dps_dat.f32_DSP_Pa/101325,-0.1902663539);
            float Lb = 0.0065;
            dps_dat.f32_Alt = (Tb*P_Pb-Tb)/(Lb*P_Pb);
            _dpsValid = true;
        }
        dps_dat.dpsValid = _dpsValid;
        update = true;
        lastDSPTime = millis();    
    } else if ((millis() - lastIMUTime > IMU_Read_Period) && !i2cBusy) {
        _imu.f32_acc_x = _myIMU->readFloatAccelX();
        _imu.f32_acc_y = _myIMU->readFloatAccelY();
        _imu.f32_acc_z = _myIMU->readFloatAccelZ();
        _imu.f32_gyro_x = _myIMU->readFloatGyroX();
        _imu.f32_gyro_y = _myIMU->readFloatGyroY();
        _imu.f32_gyro_z = _myIMU->readFloatGyroZ();
        lastIMUTime = millis();
        update = true;
    } else if (millis() - lastBATTime > BAT_Read_Period) {
        //get BAT data
        digitalWrite(VBAT_ENABLE, LOW);

        uint32_t adcCount = analogRead(PIN_VBAT);
        float adcVoltage = adcCount * VBAT_MV_PER_LBS;
        float vBat = adcVoltage * (1510.0 / 510.0);

        digitalWrite(VBAT_ENABLE, HIGH);
        _nBattPercentage = vBat / .036;
        lastBATTime = millis();
    }
    return update; 
}

void SensorSystem::setTime(DateTime date) {
    _setTime = true;
    _newDate = date;
}
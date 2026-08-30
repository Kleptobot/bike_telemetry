#include "Sensors.hpp"

namespace {
    // Observed interval between consecutive accepted samples, clamped into
    // uint16 milliseconds. A 0 result means "not yet known" (first sample,
    // or a clock that has not advanced since the previous one).
    inline uint16_t observedDt(uint32_t nowMs, uint32_t prevMs) {
        if (prevMs == 0 || nowMs <= prevMs) return 0;
        const uint32_t dt = nowMs - prevMs;
        return dt > 65000 ? 65000u : (uint16_t)dt;
    }
}

void SensorSystem::init_low() {
    _rtc.begin();
    _rtc.bit_op8(0x00, ~0x01, 0x01);

    Serial.println("RTC initialised");
    
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
    lastBATTime = millis() - BAT_Read_Period;
}

bool SensorSystem::update(bool i2cBusy) {
    
    int16_t ret;
    // getContResults() drains the ENTIRE DPS FIFO and treats the count
    // parameters as OUTPUTS only: the write bound is DPS__FIFO_SIZE (32),
    // never the arrays passed in. Buffers smaller than that are a stack
    // overflow waiting for a busy FIFO -- the previous 20-element buffers
    // were already latent (32 > 20), and shrinking them to 5 caused
    // hard-fault resets on every DPS pass. Size to what the library can
    // actually write.
    //
    // The bus-time lever is the poll period, not the buffer: results
    // accumulate at the DPS output rate, so a DPS_Read_Period of 5000 ms
    // drains ~10 results (~20 transactions) per pass, and polling faster
    // shrinks each pass further.
    uint8_t pressureCount = DPS__FIFO_SIZE;
    float pressure[pressureCount];
    uint8_t temperatureCount = DPS__FIFO_SIZE;
    float temperature[temperatureCount];
    bool update = false;

    if ((millis() - lastRTCTime > RTC_Read_Period) && !i2cBusy) {
        if (_setTime) {
            _setTime = false;
            _rtc.set(&_newDate);
        }
        #if DS3231
            _f32_RTC_Temp = _rtc.getTemperature();
        #endif
        dps_dat.f32_RTC_Temp = _f32_RTC_Temp;
        _now = _rtc.time(NULL);

        const uint32_t nowMs = millis();
        _rtcDtMs = observedDt(nowMs, lastRTCTime);
        lastRTCTime = nowMs;
        ++_rtcSeq;
        update = true;
    }
    if ((millis() - lastDSPTime > DPS_Read_Period) && !i2cBusy) {

        ret = _dps.getContResults(temperature, temperatureCount, pressure, pressureCount);
        //_dps.measureTempOnce(f32_DSP_Temp, 7);
        //_dps.measurePressureOnce(f32_DSP_Pa, 7);
        if (ret != 0)
        {
            // Serial.print("FAIL! ret = ");
            _dpsValid = false;
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
            _dpsValid = true;

            // Frame stamping: only successful fetches count as samples, so
            // the interval is success-to-success rather than attempt-to-attempt.
            const uint32_t nowMs = millis();
            _dpsDtMs = observedDt(nowMs, _dpsOkTime);
            _dpsOkTime = nowMs;
            ++_dpsSeq;
        }
        dps_dat.dpsValid = _dpsValid;
        if (dpsCallback) {
            dpsCallback({dps_dat});
        }
        update = true;
        lastDSPTime = millis();    
    }
    if ((millis() - lastIMUTime > IMU_Read_Period) && !i2cBusy) {
        // One 12-byte burst over the contiguous output registers
        // (OUTX_L_G 0x22 .. OUTZ_H_XL 0x2D) replaces six separate per-axis
        // reads: 2 bus transactions instead of 12, and all six axes carry a
        // single sample instant, which is what the fusion engine wants.
        // calc*() applies exactly the scaling readFloat*() used.
        uint8_t raw[12];
        if (_myIMU->readRegisterRegion(raw, LSM6DS3_ACC_GYRO_OUTX_L_G, sizeof(raw)) != IMU_SUCCESS) {
            // Failed read: skip the sample entirely -- no stamping, no
            // callback -- mirroring how a failed DPS fetch is handled.
        } else {
            // LSB-first 16-bit pairs: gyro block first, then accel block
            // (the same layout readRawGyro*/readRawAccel* decode).
            _imu.f32_gyro_x = _myIMU->calcGyro((int16_t)(raw[0]  | (raw[1]  << 8)));
            _imu.f32_gyro_y = _myIMU->calcGyro((int16_t)(raw[2]  | (raw[3]  << 8)));
            _imu.f32_gyro_z = _myIMU->calcGyro((int16_t)(raw[4]  | (raw[5]  << 8)));
            _imu.f32_acc_x  = _myIMU->calcAccel((int16_t)(raw[6]  | (raw[7]  << 8)));
            _imu.f32_acc_y  = _myIMU->calcAccel((int16_t)(raw[8]  | (raw[9]  << 8)));
            _imu.f32_acc_z  = _myIMU->calcAccel((int16_t)(raw[10] | (raw[11] << 8)));
            const uint32_t nowMs = millis();
            _imuDtMs = observedDt(nowMs, lastIMUTime);
            lastIMUTime = nowMs;
            ++_imuSeq;
            update = true;
            if (imuCallback) {
                imuCallback({_imu.f32_acc_z,true});
            }
        }
    }
    if (millis() - lastBATTime > BAT_Read_Period) {
        //get BAT data
        digitalWrite(VBAT_ENABLE, LOW);

        uint32_t adcCount = analogRead(PIN_VBAT);
        float adcVoltage = adcCount * VBAT_MV_PER_LBS;
        float vBat = adcVoltage * (1510.0 / 510.0);

        digitalWrite(VBAT_ENABLE, HIGH);

        // Map the cell's usable range, not 0 V to 3.6 V.
        //
        // The previous formula was `vBat / 0.036f`, a linear ramp from zero
        // volts reaching 100% at 3.6 V. For the LiPo this board actually uses
        // that reads 100% all the way from 4.2 V down to 3.6 V -- roughly a
        // quarter of the remaining charge -- and still reports 83% at 3.0 V,
        // by which point the cell is empty. The gauge never left the top of
        // its range, so it carried no information.
        //
        // A straight line over 3.30 V - 4.20 V is still an approximation of a
        // LiPo discharge curve, which is flat in the middle and steep at both
        // ends, but it is monotonic across the range that matters and reaches
        // 0% at a sensible cutoff.
        _nBattPercentage = (int)constrain((vBat - BAT_EMPTY_V) * 100.0f
                                              / (BAT_FULL_V - BAT_EMPTY_V),
                                          0.0f, 100.0f);

        // Measurement-frame stamping: the voltage is the actual measurement
        // (the percentage above is derived from it), and the charge-state
        // input is read alongside. BAT_CHARGE_STATE is LOW while charging.
        _vBatVolts = vBat;
        _charging  = (digitalRead(BAT_CHARGE_STATE) == LOW);
        const uint32_t nowMs = millis();
        _batDtMs = observedDt(nowMs, lastBATTime);
        lastBATTime = nowMs;
        ++_batSeq;
    }
    return update; 
}

void SensorSystem::setTime(struct tm date) {
    _setTime = true;
    _newDate = date;
}
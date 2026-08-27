#include "HAL.hpp"
#include "DebugConfig.hpp"
#include <numeric>

void HAL::init_low() {
    inputSystem.init();
    sensorSystem.init_low();
    _resetGPSTime = 0;
    _sleep = false;
}

void HAL::init(timeData* date) {

    HAL::inst().enableAuxRail();
    //turn the gps power supply on
    inputSystem.setOutput(GPIOB3, true);
    inputSystem.update(false);
    _LC76G.i2c_wait();

    Wire.begin();
    Wire.setClock(100000); // 100kHz
    sensorSystem.init();
    bluetoothSystem.init(&storageSystem);
    for (int i=0; i<5; i++) {
        inputSystem.update(false);
        delay(50);
    }
    if (!inputs().SD_Det.state) {   //SD card pin is inverted, low means card is present
        while (!storageSystem.init(date)) {
            Serial.println("SD card detected, initializing...");
            delay(200);
        }
    } else {
        Serial.println("No SD card detected.");
    }
    _LC76G.begin(&Wire);

    //set up some callbacks
    sensorSystem.onDPS([this](dps_data dps) {
        f32_altitude = altFusion.computeAltitudeFromPressure(dps.f32_DSP_Pa, dps.f32_DSP_Temp);
        if (altFusion.seaLevelPressureCalibrated()) {
            altFusion.altitudeDPSUpdate(f32_altitude);
        }
        _dpsValid = dps.dpsValid;
    });
    sensorSystem.onIMU([this](data_record acc_z) {
        altFusion.altitudeIMUUpdate(-1*acc_z.value);
    });
}

uint32_t lastHdopCheck = 0, lastbaroCalibration = 0;;
void HAL::update() {
    _LC76G.update();

    //check the age of the gps altitude measurement
    if (millis() - lastHdopCheck > 1000 && _LC76G.gps().hdop.isValid()) {
        Serial.print("HDOP: "); Serial.println(_LC76G.gps().hdop.hdop());
        Serial.print("GPS Altitude: "); Serial.println(_LC76G.gps().altitude.meters());
        Serial.print("Baro Altitude: ");
        Serial.println(altFusion.computeAltitudeFromPressure(sensorSystem.dps().f32_DSP_Pa, sensorSystem.dps().f32_DSP_Temp));
        Serial.print("IMU Acc Z: "); Serial.println(sensorSystem.imu().f32_acc_z);
        Serial.print("altFusion Altitude: "); Serial.println(altFusion.altitude());
        Serial.print("altFusion Vertical Velocity: "); Serial.println(altFusion.rise());
        lastHdopCheck = millis();

        if (_LC76G.gps().altitude.isValid() && _LC76G.gps().hdop.hdop() < 2.0f) {
            altFusion.baroCalibrateFromGPS(_LC76G.gps().altitude.meters(), sensorSystem.dps().f32_DSP_Pa, sensorSystem.dps().f32_DSP_Temp);
        }
    }

    if (_LC76G.gps().altitude.age() < gpsAltAge) {
        altFusion.altitudeGPSCorrect(_LC76G.gps().altitude.meters());
    }
    gpsAltAge = _LC76G.gps().altitude.age();

    //check the age of the gps speed measurement
    auto gpsSpd = _LC76G.gps().speed;
    if (gpsSpd.age() < lastGPSSpdUpdate) {
        //speedRecord.gps = {(float)gpsSpd.kmph(),millis(),true};
    }
    lastGPSSpdUpdate = gpsSpd.age();

    //Call GPIO inputs, pass in busy state of i2c
    if(inputSystem.update(_LC76G.isBusy())) {
        //on success tell the LC76G to delay
        _LC76G.i2c_wait();
    }
    
    //Call sensors, pass in busy state of i2c
    if(sensorSystem.update(_LC76G.isBusy())) {
        //on success tell the LC76G to delay
        _LC76G.i2c_wait();
    }
    bluetoothSystem.update();

    if (_dpsValid) {
        f32_alt = altFusion.altitude();
    } else {
        f32_alt = _LC76G.gps().altitude.meters();
    }

    // Ambient temperature. This was never assigned anywhere, so getTemperature()
    // returned a constant 0.0 (HAL is a function-local static, so zero-init
    // rather than garbage -- which is why it looked plausible on screen).
    // The DPS368 reading is the better source; fall back to the RTC's own
    // sensor when the barometer has not produced a valid sample.
    const dps_data& dps = sensorSystem.dps();
    if (dps.dpsValid) {
        f32_temp = dps.f32_DSP_Temp;
    } else {
        f32_temp = dps.f32_RTC_Temp;
    }
    
    wheelRPM = csc::getSpeed();
    gpsKmh = {0, false};
    if (gpsSpd.isValid()) {
        gpsKmh = {(float)gpsSpd.kmph(), true};
    }

    f32_cadence = 0;
    if (csc::getCadence().live) {
        f32_cadence = csc::getCadence().value;
    }
    
    f32_bpm = 0;
    if (hrm::getHRM().live) {
        f32_bpm = hrm::getHRM().value;
    }

    f32_pow = 0;
    if (cps::getPower().live) {
        f32_pow = cps::getPower().value;
    }

    //if reset time is non zero check if 100ms has passed since the trigger, then reset time to zero and write reset pin high
    if (_resetGPSTime > 0) {
        if (millis() - _resetGPSTime > 100) {
            inputSystem.setOutput(GPIOB5, true);
            _resetGPSTime = 0;
        }
    }
    if (_resetDispTime > 0) {
        if (millis() - _resetDispTime > 100) {
            inputSystem.setOutput(GPIOB7, true);
            _resetDispTime = 0;
        }
    }
}

void HAL::resetGPS() {
    inputSystem.setOutput(GPIOB5, false);
    _resetGPSTime = millis();
}

void HAL::buzzStart() {
    inputSystem.setOutput(GPIOB2, true);
}

void HAL::buzzStop() {
    inputSystem.setOutput(GPIOB2, false);
}

void HAL::sleep() {
    Serial.println("sleep");
    disableAuxRail();
    if (!_sleep) {
        _sleep = true;
        _LC76G.sendCommand(LC76G::PAIR_LOW_POWER_ENTRY_RTC_MODE,&HAL::onSleep,this,nullptr);
    }
}

void HAL::setNMEArates(uint8_t type, uint8_t rate) {
    LC76G::Payload2U8 p = {type, rate};
    _LC76G.sendCommand(LC76G::SET_NMEA_MSG_RATE,&HAL::onPAIRResponse,this,&p);
}

void HAL::onSleep(int numArgs, const void* payload, void* context) {
    auto* self = static_cast<HAL*>(context);
    self->_LC76G.closeDataFile();
    self->inputSystem.setOutput(GPIOB3,false);    //turn off the GPS enable supply
    self->inputSystem.setOutput(GPIOB6,false);    //turn off the screen backlight
}

void HAL::onPAIRResponse(int numArgs, const void* payload, void* context) {
    auto* self = static_cast<HAL*>(context);

    self->handlePAIRResponse(numArgs, payload);
}

void HAL::handlePAIRResponse(int numArgs, const void* payload) {
    // Validate payload before using it
    if (payload == nullptr || numArgs <= 0) {
        if (ENABLE_GPS_DEBUG) {
            Serial.println("[HAL] PAIR response received with no payload");
        }
        return;
    }
    
    uint8_t* byte_array = (uint8_t*)payload;
    if (ENABLE_GPS_DEBUG) {
        for(int i=0; i<numArgs && i<5; i++) {  // Added bounds check (max 5 bytes)
            Serial.print("[PAIR] Arg ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(byte_array[i]);
        }
    }
}
void HAL::disableAuxRail() {
  // Stop hardware SPI first so it releases its pin drive
  SPI.end();

  // Explicitly float the shared bus lines so no output stage
  // can backfeed AUX_3V3 through internal clamp diodes
  pinMode(D0,   INPUT); // TFT_CS
  pinMode(D1,   INPUT); // SD_CS
  pinMode(D2,   INPUT); // TFT_DC
  pinMode(D4,   INPUT); // 
  pinMode(D5,   INPUT); // 
  pinMode(D7,   INPUT); // FLASH_CS
  pinMode(D9,   INPUT); // MISO
  pinMode(D10,  INPUT); // MOSI

  // Now safe to cut the rail
  digitalWrite(D6, LOW);
}

void HAL::enableAuxRail() {
  digitalWrite(D6, HIGH);
  delay(5); // allow AUX_3V3 to stabilize before driving the flash chip

  // Restore CS as output, deasserted (idle high for most SPI flash)
  pinMode(D0,   OUTPUT); // TFT_CS
  pinMode(D1,   OUTPUT); // SD_CS
  pinMode(D2,   OUTPUT); // TFT_DC
  pinMode(D7,   OUTPUT); // FLASH_CS
  pinMode(D9,   OUTPUT); // MISO
  pinMode(D10,  OUTPUT); // MOSI

  // Re-init hardware SPI for use
  SPI.begin();
}
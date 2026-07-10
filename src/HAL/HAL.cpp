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

    //reset some systems
    resetDisplay();

    inputSystem.setOutput(GPIOB6,true); //turn on the screen backlight

    //set up some callbacks
    sensorSystem.onDPS([this](data_record alt) {
        altFusion.altitudeDPSUpdate(alt.value);
        _dpsValid = alt.live;
    });
    sensorSystem.onIMU([this](data_record acc_z) {
        altFusion.altitudeIMUUpdate(acc_z.value);
    });
}

void HAL::update() {
    _LC76G.update();

    //check the age of the gps altitude measurement
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
    
    auto spd = csc::getSpeed();
    f32_kmh = 0;
    if (spd.live) {
        f32_kmh = spd.value;
    } else if (gpsSpd.isValid()) {
        f32_kmh = gpsSpd.kmph();
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

    f32_grade = 0;
    if (f32_kmh >0)
    {
        //rise in m/s * 3.6 to convert to km/h, then divide by speed in km/h to get grade as a percentage
        f32_grade = altFusion.rise()*3.6f*100.0f/f32_kmh;
    }
    
}

void HAL::resetGPS() {
    inputSystem.setOutput(GPIOB5, false);
    _resetGPSTime = millis();
}

void HAL::resetDisplay() {
    inputSystem.setOutput(GPIOB7, false);
    _resetDispTime = millis();
}

void HAL::buzzStart() {
    inputSystem.setOutput(GPIOB2, true);
}

void HAL::buzzStop() {
    inputSystem.setOutput(GPIOB2, false);
}

void HAL::sleep() {
    Serial.println("sleep");
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
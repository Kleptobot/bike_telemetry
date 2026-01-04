#include "HAL.hpp"
#include <numeric>

void HAL::init_low() {
    inputSystem.init();
    sensorSystem.init_low();
    _resetGPSTime = 0;
    _sleep = false;
}

void HAL::init() {
    //turn the gps power supply on
    inputSystem.setOutput(GPIOB3, true);
    inputSystem.update(false);
    _LC76G.i2c_wait();

    Wire.begin();
    Wire.setClock(100000); // 100kHz
    sensorSystem.init();
    bluetoothSystem.init(&storageSystem);
    while (!storageSystem.init()) {
        delay(200);
        Serial.println("retrying...");
    }
    _LC76G.begin(&Wire);

    //reset some systems
    resetDisplay();

    inputSystem.setOutput(GPIOB6,true); //turn on the screen backlight

    
}

void HAL::update() {
    _LC76G.update();

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

    //agregate speed from gps and sensors
    f32_kph = 0;
    std::vector<float> speed = csc::getSpeed();
    if(_LC76G.gps().speed.isValid()) {
        speed.push_back(_LC76G.gps().speed.kmph());
    }
    if(speed.size()>0) {
        f32_kph = std::accumulate(speed.begin(), speed.end(), 0.0f)/speed.size();
    }

    //agregate cadence from sensors
    std::vector<float> cadence = csc::getCadence();
    f32_cadence = 0;
    if(cadence.size()>0) {
        f32_cadence = std::accumulate(cadence.begin(), cadence.end(), 0.0f)/cadence.size();
    }

    //agregate heartrate from sensors
    std::vector<float> heartrates = hrm::getHRM();
    f32_bpm = 0;
    if(heartrates.size()>0) {
        f32_bpm = std::accumulate(heartrates.begin(), heartrates.end(), 0.0f)/heartrates.size();
    }

    //agregate power from sensors
    std::vector<float> pow = cps::getPower();
    if(pow.size()>0) {
        f32_pow = std::accumulate(pow.begin(), pow.end(), 0.0f)/pow.size();
    }

    //agregate altitude from gps and dps
    f32_alt = 0;
    std::vector<float> altitudes;
    if (sensorSystem.dps().dpsValid) {
        altitudes.push_back(sensorSystem.dps().f32_Alt);
    }
    if (_LC76G.gps().altitude.isValid()) {
        altitudes.push_back(_LC76G.gps().altitude.meters());
    }
    if (altitudes.size()>0) {
        f32_alt = std::accumulate(altitudes.begin(), altitudes.end(), 0.0f) / altitudes.size();
    }

    f32_temp = (sensorSystem.dps().f32_DSP_Temp + sensorSystem.dps().f32_RTC_Temp)/2;

    //if the telemetry callback has been set, run it
    if (telemetryCallback) {
        telemetryCallback(sensorSystem.imu(),
                          sensorSystem.dps(),
                          f32_kph,
                          f32_cadence,
                          f32_temp,
                          f32_alt,
                          f32_bpm,
                          f32_pow,
                          _LC76G.gps().location,
                          sensorSystem.now());
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

void HAL::resetDisplay() {
    inputSystem.setOutput(GPIOB7, false);
    _resetDispTime = millis();
}

void HAL::displayGPSInfo() {
  Serial.println("\n--- GPS Status ---");
  
  if (_LC76G.gps().location.isValid()) {
    Serial.print("Lat: ");
    Serial.print(_LC76G.gps().location.lat(), 6);
    Serial.print(" Lng: ");
    Serial.println(_LC76G.gps().location.lng(), 6);
    Serial.print("Alt: ");
    Serial.print(_LC76G.gps().altitude.meters());
    Serial.println(" m");

  } else {
    Serial.println("Waiting for gps fix...");
  }
  
  if (_LC76G.gps().satellites.isValid()) {
    Serial.print("Satellites: ");
    Serial.println(_LC76G.gps().satellites.value());
  }
  
  if (_LC76G.gps().speed.isValid()) {
    Serial.print("Speed: ");
    Serial.print(_LC76G.gps().speed.kmph());
    Serial.println(" km/h");
  }
}

void HAL::sleep() {
    Serial.println("sleep");
    if (!_sleep) {
        _sleep = true;
        _LC76G.sendCommand(LC76G::PAIR_LOW_POWER_ENTRY_RTC_MODE,&HAL::onSleep,this,nullptr);
    }
}

void HAL::getNMEArates(uint8_t type) {
    LC76G::Payload1U8 p = {type};
    _LC76G.sendCommand(LC76G::GET_NMEA_MSG_RATE,&HAL::onPAIRResponse,this,&p);
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
    uint8_t* byte_array = (uint8_t*)payload;
    for(int i=0; i<numArgs; i++) {
        Serial.println(byte_array[i]);
    }
}
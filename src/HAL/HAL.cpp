#include "HAL.hpp"

LC76G HAL::_LC76G;
InputSystem HAL::inputSystem;
SensorSystem HAL::sensorSystem;
TinyGPSPlus HAL::_gps;
SDCardSystem HAL::storageSystem;

HAL::TelemetryCallback HAL::telemetryCallback;

float HAL::f32_kph, HAL::f32_cadence, HAL::f32_temp, HAL::f32_alt, HAL::f32_bpm;
float HAL::f32_GPS_speed, HAL::f32_GPS_Alt;
uint8_t HAL::_rxBuffer[1024];
uint32_t HAL::_resetTime;
LC76G::State HAL::lc76g_state_prev;
bool HAL::_sleep;

void HAL::init_low() {
    inputSystem.init();
    sensorSystem.init_low();
    _resetTime = 0;
    _sleep = false;
}

void HAL::init() {
    //turn the gps power supply on
    inputSystem.setOutput(GPIOB3, true);
    inputSystem.update(false);
    _LC76G.i2c_wait();

    Wire.begin();
    Wire.setClock(50000); // 50kHz
    _LC76G.begin(&Wire);
    sensorSystem.init();
    bluetoothSystem.init();
    storageSystem.init();
    resetGPS();
}

void HAL::update() {
    uint16_t receivedLength;
    LC76G::State lc76g_state = _LC76G.update();
    if (lc76g_state == LC76G::State::STATE_COMPLETE_RECEIVE) {
        // Feed received data to GPS parser
        receivedLength = _LC76G.getReceivedData(_rxBuffer, 1024);
        for (uint16_t i = 0; i < receivedLength; i++) {
            _gps.encode(_rxBuffer[i]);
        }
    }
    else if(lc76g_state == LC76G::State::STATE_COMPLETE_TRANSMIT) {
        ; //put a callback here when a sleep transmit has been queued
        if(_sleep)
        {
            inputSystem.setOutput(GPIOB3,false);
            _sleep = false;
        }
    }
    lc76g_state_prev = lc76g_state;

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

    if(_gps.speed.isValid()) 
      f32_GPS_speed = _gps.speed.kmph();
    if(_gps.altitude.isValid())
      f32_GPS_Alt = _gps.altitude.meters();

    //agregate speed from gps and sensors
    std::vector<float> speed = csc::getSpeed();
    if(_gps.speed.isValid()) {
        speed.push_back(f32_GPS_speed);
    }
    f32_kph = 0;
    for (auto it = speed.begin(); it != speed.end(); it++) {
        f32_kph += (*it);
    }
    if(speed.size()>0)
    f32_kph = f32_kph/speed.size();

    //agregate cadence from sensors
    std::vector<float> cadence = csc::getCadence();
    f32_cadence = 0;
    for (auto it = cadence.begin(); it != cadence.end(); it++) {
        f32_cadence += (*it);
    }
    if(cadence.size()>0)
        f32_cadence = f32_cadence/cadence.size();

    //agregate heartrate from sensors
    std::vector<float> heartrates = hrm::getHRM();
    f32_bpm = 0;
    for (auto it = heartrates.begin(); it != heartrates.end(); it++) {
        f32_bpm += (*it);
    }
    if(heartrates.size()>0)
        f32_bpm = f32_bpm/heartrates.size();

    //agregate altitude from gps and dps
    f32_alt = sensorSystem.dps().f32_Alt;
    if (_gps.altitude.isValid()) {
        f32_alt = (f32_alt+_gps.altitude.meters())/2;
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
                          _gps.location,
                          sensorSystem.now());
    }

    //if reset time is non zero check if 100ms has passed since the trigger, then reset time to zero and write reset pin high
    if (_resetTime > 0) {
        if (millis() - _resetTime > 100) {
            inputSystem.setOutput(GPIOB5, true);
            _resetTime = 0;
        }
    }


}

void HAL::resetGPS() {
    inputSystem.setOutput(GPIOB5, false);
    _resetTime = millis();
}

void HAL::displayGPSInfo() {
  Serial.println("\n--- GPS Status ---");
  
  if (_gps.location.isValid()) {
    Serial.print("Lat: ");
    Serial.print(_gps.location.lat(), 6);
    Serial.print(" Lng: ");
    Serial.println(_gps.location.lng(), 6);
    Serial.print("Alt: ");
    Serial.print(_gps.altitude.meters());
    Serial.println(" m");

  } else {
    Serial.println("Waiting for gps fix...");
  }
  
  if (_gps.satellites.isValid()) {
    Serial.print("Satellites: ");
    Serial.println(_gps.satellites.value());
  }
  
  if (_gps.speed.isValid()) {
    Serial.print("Speed: ");
    Serial.print(_gps.speed.kmph());
    Serial.println(" km/h");
  }
}

void HAL::sleep() {
    _sleep = true;
    const char* cmd="$PAIR650,0*25";
    _LC76G.queueCommand((uint8_t*)cmd, strlen(cmd));
}
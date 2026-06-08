// HAL.h
#pragma once

#include <TinyGPSPlus.h>
#include <functional>

#include "InputSystem.hpp"
#include "Sensors.hpp"
#include "Bluetooth/BluetoothSystem.hpp"
#include "LC76G.hpp"
#include "SDCard.hpp"

class HAL {
  public:
    static HAL& inst() {
        static HAL instance;
        return instance;
    }

    void init_low();
    void init();
    void update();
    physIO inputs() { return inputSystem.state(); };
    IStorage* SD() { return &storageSystem; }
    BluetoothSystem& bluetooth() { return bluetoothSystem; }
    void reInitStorage() {
      while (!storageSystem.init()) {
            Serial.println("SD card detected, initializing...");
            delay(200);
        }
    }
   
    void displayGPSInfo();
    void sleep();
    void getNMEArates(uint8_t type);
    void setNMEArates(uint8_t type, uint8_t rate);
    void gpsRestoreDefaults() { _LC76G.sendCommand(LC76G::RESTORE_DEFAULT_SETTING,nullptr,this,nullptr); }
    void gpsSaveNVRAM() { _LC76G.sendCommand(LC76G::SAVE_TO_NVRAM,nullptr,this,nullptr); }
    void gpsHotStart() { _LC76G.sendCommand(LC76G::GNSS_SUBSYS_HOT_START,nullptr,this,nullptr); }
    void gpsWarmStart() { _LC76G.sendCommand(LC76G::GNSS_SUBSYS_WARM_START,nullptr,this,nullptr); }
    void gpsColdStart() { _LC76G.sendCommand(LC76G::GNSS_SUBSYS_COLD_START,nullptr,this,nullptr); }
    void setRMCRate() { 
      LC76G::Payload1Ch1U8 p = {"RMC",1};
      _LC76G.sendCommand(LC76G::SET_NMEA_RATE, &HAL::onPAIRResponse, this, &p);
    }
    void resetGPS();

    using TelemetryCallback = std::function<void(imu_data imu, dps_data dps, int BattPercentage, float speed, float cadence, float temp, float alt, float bpm, float pow, TinyGPSLocation loc, DateTime now)>;

    void onTelemetry(TelemetryCallback cb) { telemetryCallback = cb; }

    void setTime(DateTime date) { sensorSystem.setTime(date); }

    void buzzStart();
    void buzzStop();

  private:
    HAL() : _LC76G(storageSystem) {}

    //internal HAL systems
    InputSystem inputSystem;
    SensorSystem sensorSystem;
    BluetoothSystem bluetoothSystem;
    SDCardSystem storageSystem;
    LC76G _LC76G;

    //callbacks used to transmit data to the App
    TelemetryCallback telemetryCallback;
    // static BluetoothCallback bluetoothCallback;

    // private memeber variables
    float f32_kph, f32_cadence, f32_temp, f32_alt, f32_bpm, f32_pow;
    uint8_t _rxBuffer[1024];
    uint32_t _resetGPSTime, _resetDispTime;
    bool _sleep;

    //private methods
    void resetDisplay();
    static void onSleep(int numArgs, const void* payload, void* context);
    static void onPAIRResponse(int numArgs, const void* payload, void* context);
    void handlePAIRResponse(int numArgs, const void* payload);
};
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
   
    void displayGPSInfo();
    void sleep();
    void setRCM(int mode);
    void getNMEArates();

    using TelemetryCallback = std::function<void(imu_data imu, dps_data dps, float speed, float cadence, float temp, float alt, float bpm, float pow, TinyGPSLocation loc, DateTime now)>;

    void onTelemetry(TelemetryCallback cb) { telemetryCallback = cb; }

    void setTime(DateTime date) { sensorSystem.setTime(date); }

  private:
    HAL() {}

    //internal HAL systems
    LC76G _LC76G;
    InputSystem inputSystem;
    SensorSystem sensorSystem;
    BluetoothSystem bluetoothSystem;
    SDCardSystem storageSystem;

    //callbacks used to transmit data to the App
    TelemetryCallback telemetryCallback;
    // static BluetoothCallback bluetoothCallback;

    // private memeber variables
    float f32_kph, f32_cadence, f32_temp, f32_alt, f32_bpm, f32_pow;
    uint8_t _rxBuffer[1024];
    uint32_t _resetGPSTime, _resetDispTime;
    bool _sleep;

    //private methods
    void resetGPS();
    void resetDisplay();
    static void onSleep(const char* sentence, uint16_t length, void* context);
};
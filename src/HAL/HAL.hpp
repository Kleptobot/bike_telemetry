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
    static void init_low();
    static void init();
    static void update();
    static physIO inputs() { return inputSystem.state(); };
    static IStorage* SD() { return &storageSystem; }
    static BluetoothSystem& bluetooth() { return bluetoothSystem; }
   
    static void displayGPSInfo();
    static void sleep();

    using TelemetryCallback = std::function<void(imu_data imu, dps_data dps, float speed, float cadence, float temp, float alt, float bpm, TinyGPSLocation loc, DateTime now)>;

    static void onTelemetry(TelemetryCallback cb) { telemetryCallback = cb; }

    static void setTime(DateTime date) { sensorSystem.setTime(date); }

  private:
    //internal HAL systems
    static LC76G _LC76G;
    static TinyGPSPlus _gps;
    static InputSystem inputSystem;
    static SensorSystem sensorSystem;
    static BluetoothSystem bluetoothSystem;
    static SDCardSystem storageSystem;

    //callbacks used to transmit data to the App
    static TelemetryCallback telemetryCallback;
    // static BluetoothCallback bluetoothCallback;

    // private memeber variables
    static float f32_kph, f32_cadence, f32_temp, f32_alt, f32_bpm;
    static float f32_GPS_speed, f32_GPS_Alt;
    static uint8_t _rxBuffer[1024];
    static uint32_t _resetTime;
    static LC76G::State lc76g_state_prev;
    static bool _sleep;

    //private methods
    static void resetGPS();
};
#ifndef HAL_H
#define HAL_H

#include <TinyGPSPlus.h>

#include "InputSystem.hpp"
#include "Sensors.hpp"
#include "Bluetooth/BluetoothSystem.hpp"
#include "LC76G.hpp"
#include "SDCard.hpp"
#include "AltitudeFusion.hpp"

class HAL {
    public:
    static HAL& inst() {
        static HAL instance;
        return instance;
    }

    void init_low();
    void init(timeData* date);
    void update();
    physIO inputs() { return inputSystem.state(); };
    IStorage* SD() { return &storageSystem; }
    BluetoothSystem& bluetooth() { return bluetoothSystem; }
    void reInitStorage(timeData* date) {
        while (!storageSystem.init(date)) {
            Serial.println("SD card detected, initializing...");
            delay(200);
        }
    }

    void sleep();
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

    bool SDMounted() const { return storageSystem.isMounted(); };
    void unMountSD() {storageSystem.unMount(); };

    void setTime(DateTime date) { sensorSystem.setTime(date); }

    void buzzStart();
    void buzzStop();

    int16_t getBatteryPercentage() const { return sensorSystem.batt(); }
    float getSpeed() const { return f32_kmh; }
    float getCadence() const { return f32_cadence; }
    float getTemperature() const { return f32_temp; }
    float getAltitude() const { return f32_alt; }
    float getHeartRate() const { return f32_bpm; }
    float getPower() const { return f32_pow; }
    float getGrade() const { return f32_grade; }

    TinyGPSLocation getGPSLocation() const { return _LC76G.gps().location; }
    TinyGPSTime getGPSTime() const { return _LC76G.gps().time; }
    DateTime getRTCtime() const { return sensorSystem.now(); }
    imu_data getIMUData() const { return sensorSystem.imu(); }
    dps_data getDPSData() const { return sensorSystem.dps(); }


    private:
    HAL() : _LC76G(storageSystem) {}

    //internal HAL systems
    InputSystem inputSystem;
    SensorSystem sensorSystem;
    BluetoothSystem bluetoothSystem;
    SDCardSystem storageSystem;
    LC76G _LC76G;
    AltitudeFusion altFusion;

    // private memeber variables
    float f32_kmh, f32_cadence, f32_temp, f32_alt, f32_bpm, f32_pow, f32_grade;
    uint8_t _rxBuffer[1024];
    uint32_t _resetGPSTime, _resetDispTime;
    bool _sleep;
    uint32_t gpsAltAge = 0;
    uint32_t lastGPSSpdUpdate = 0;
    bool _dpsValid = false;

    //private methods
    void resetDisplay();
    static void onSleep(int numArgs, const void* payload, void* context);
    static void onPAIRResponse(int numArgs, const void* payload, void* context);
    void handlePAIRResponse(int numArgs, const void* payload);

    //
};

#endif /* HAL_H */
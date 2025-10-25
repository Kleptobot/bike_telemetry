#pragma once
#include "UI/UIManager.hpp"
#include "HAL/HAL.hpp"
#include "TCXLogger.hpp"
#include "AppEvents.hpp"
#include "DataModel.hpp"
#include "HAL/BluetoothInterface.hpp"

// Application-level states
enum class AppState {
    BOOT,


    IDLE,
    LOGGING,
    PAUSED
};

class App : public UIEventBus{
public:
    static App& instance() {
        static App instance;
        return instance;
    }

    DataModel& dataModel() { return model; }

    // Called at startup
    void begin(UIManager* ui, IStorage* storage);

    void postAppEvent(const AppEvent& e) override {
        appEvents.push(e);
    }

    void postUIEvent(const UIEvent& e) override {
        ui->handleUIEvent(e);
    }

    // Called from loop()
    void update();

    // Data from HAL
    void updateTelemetry(imu_data imu, dps_data dps, float speed, float cadence, float temp, float alt, float bpm, TinyGPSLocation loc, DateTime now);
    void updateBluetooth(std::vector<BluetoothDevice> devices);
    void updateGpsEnable(bool state);

    // Getters for UI access
    AppState getState() const { return state; }
    bool getGpsEnableState() const { return _gpsEnableState; }

    bool isLogging() const { return state == AppState::LOGGING; }

    void handleAppEvent(const AppEvent& e);

private:
    App() = default; // private singleton

private:
    std::queue<AppEvent> appEvents;
    UIManager* ui = nullptr;
    IStorage* _storage = nullptr;
    TCXLogger* logger = nullptr;
    AppState state = AppState::BOOT;

    float lastSpeed, f32_distance;
    dps_data currentDps;

    imu_data currentIMU;
    physIO _inputs;
    bool _gpsEnableState = true;

    uint8_t lastSecond;

    DataModel model;
};
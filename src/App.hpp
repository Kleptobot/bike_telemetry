#pragma once
#include "UI/UIManager.hpp"
#include "HAL/HAL.hpp"
#include "TCXLogger.hpp"

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
    void updateTelemetry(imu_data imu, float speed, float cadence, float temp, float alt, float bpm, TinyGPSLocation loc, DateTime now);
    void updateBluetooth(std::vector<BluetoothDevice> devices);
    void updateGpsEnable(bool state);

    // Getters for UI access
    float getSpeed() const { return currentSpeed; }
    float getCadence() const { return currentCadence; }
    float getTemp()  const { return currentTemp; }
    float getAlt() const { return currentAlt; }
    float getBPM() const { return currentBPM; }
    DateTime now() const { return currentTime; }
    imu_data getIMU() const { return currentIMU; }
    AppState getState() const { return state; }
    TinyGPSLocation getLoc() const { return currentLocation; }
    const std::vector<BluetoothDevice>& getDevices() const { return _devices; }
    physIO getInput() const { return _inputs; }
    bool getGpsEnableState() const { return _gpsEnableState; }

    bool isLogging() const { return state == AppState::LOGGING; }

    void handleAppEvent(const AppEvent& e);

private:
    App() = default; // private singleton
    void transitionTo(AppState newState);

private:
    std::queue<AppEvent> appEvents;
    UIManager* ui = nullptr;
    IStorage* _storage = nullptr;
    TCXLogger* logger = nullptr;
    AppState state = AppState::BOOT;

    float currentSpeed = 0.0f;
    float currentCadence = 0.0f;
    float currentTemp = 0.0f;
    float currentAlt = 0.0f;
    float currentBPM = 0.0f;
    DateTime currentTime;
    float lastSpeed, f32_distance;

    TinyGPSLocation currentLocation;
    imu_data currentIMU;
    std::vector<BluetoothDevice> _devices;
    physIO _inputs;
    bool _gpsEnableState = true;

    uint8_t lastSecond;
};
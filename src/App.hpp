#pragma once
#include "UI/UIManager.hpp"
#include "HAL/HAL.hpp"
#include "TCXLogger.hpp"
#include "AppEvents.hpp"
#include "DataModel/DataModel.hpp"
#include "HAL/BluetoothInterface.hpp"

// Application-level states

class App : public UIEventBus{
public:
    static App& instance() {
        static App instance;
        return instance;
    }

    DataModel& dataModel() { return model; }

    // Called at startup
    void begin(IStorage* storage);

    void postAppEvent(const AppEvent& e) override {
        appEvents.push(e);
    }

    void postUIEvent(const UIEvent& e) override {
        ui.handleUIEvent(e);
    }

    // Called from loop()
    void update();

    // Data from HAL
    void updateTelemetry(imu_data imu, dps_data dps, float speed, float cadence, float temp, float alt, float bpm, float pow, TinyGPSLocation loc, DateTime now);
    void updateBluetooth(std::vector<BluetoothDevice> devices);
    void updateGpsEnable(bool state);

    // Getters for UI access
    AppState getState() const { return state; }
    bool getGpsEnableState() const { return _gpsEnableState; }

    bool isLogging() const { return state == AppState::LOGGING; }

    void handleAppEvent(const AppEvent& e);
    
    const DataModel& getModel() const { return model; }
    DataModel& getModel() { return model; }

private:
    App() : model(), ui(*this) {}

private:
    std::queue<AppEvent> appEvents;
    IStorage* _storage = nullptr;
    TCXLogger* logger = nullptr;
    AppState state = AppState::BOOT, state_prev;
    TinyGPSLocation _lastLocation;
    uint32_t _lastSeconds;

    bool _gpsEnableState = true;

    uint8_t lastSecond;

    DataModel model;
    UIManager ui;
    uint32_t _millis, _last_millis, lastGPS;

    void saveBiometrics();
    void loadBiometrics();

    void saveLayout();
    void loadLayout();
};
#ifndef APP_H
#define APP_H

#include "UI/UIManager.hpp"
#include "HAL/HAL.hpp"
#include "Loggers/TCXLogger.hpp"
#include "Loggers/FITLogger.hpp"
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
    void updateTelemetry(imu_data imu, dps_data dps, int16_t BattPercentage, float speed, float cadence, float temp, float alt, float bpm, float pow, TinyGPSLocation loc, DateTime now, TinyGPSTime gpsNow);
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
    
    std::queue<AppEvent> appEvents;
    IStorage* _storage = nullptr;
    ILogger* _logger = nullptr;

    TCXLogger* tcxLogger = nullptr;
    FITLogger* fitLogger = nullptr;

    AppState state = AppState::BOOT, state_prev;
    TinyGPSLocation _lastLocation;
    DateTime _gpsNow;
    bool _gpsNowValid = false;
    uint32_t _lastSeconds;
    uint32_t _lastRenderMillis = 0;
    int messageType = 0;
    uint32_t _messageSendMillis = 0;
    bool startMessageConfig = false;

    bool _gpsEnableState = true;
    bool validLoc_prev = false;

    uint8_t lastSecond;

    DataModel model;
    UIManager ui;
    uint32_t _millis, _last_millis, lastGPS;

    void saveBiometrics();
    void loadBiometrics();

    void saveLayout();
    void loadLayout();
};

#endif /* APP_H */
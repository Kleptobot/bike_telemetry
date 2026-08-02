#ifndef APP_H
#define APP_H

#include "UI/UIManager.hpp"
#include "HAL/HAL.hpp"
#include "Loggers/TCXLogger.hpp"
#include "Loggers/FITLogger.hpp"
#include "Loggers/CSVLogger.hpp"
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
    void updateTelemetry();
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
    CSVLogger* csvLogger = nullptr;

    AppState state = AppState::BOOT, state_prev = AppState::BOOT;
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

    void saveBikeStats();
    void loadBikeStats();

    void saveLayout();
    void loadLayout();

    void saveTime();
    void loadTime();

    void updateLoggerInstance() {
            const LoggerType selectedLogger = model.bike().get().logger;

            //delete previously created loggers if they exist
            if (fitLogger) {
                delete fitLogger;
                fitLogger = nullptr;
            }
            if (tcxLogger) {
                delete tcxLogger;
                tcxLogger = nullptr;
            }
            if (csvLogger) {
                delete csvLogger;
                csvLogger = nullptr;
            }

            switch (selectedLogger) {
                case LoggerType::FIT:
                    fitLogger = new FITLogger(_storage);
                    _logger = fitLogger;
                    Serial.println("FIT logger initialized.");
                    break;
                case LoggerType::TCX:
                    tcxLogger = new TCXLogger(_storage, model);
                    _logger = tcxLogger;
                    Serial.println("TCX logger initialized.");
                    break;
                case LoggerType::CSV:
                    // Implement CSV logger initialization if needed
                    csvLogger = new CSVLogger(_storage);
                    _logger = csvLogger;
                    Serial.println("CSV logger initialized.");
                    break;
            }
    }
};

#endif /* APP_H */
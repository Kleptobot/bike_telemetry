#ifndef APP_H
#define APP_H

#include "UI/UIManager.hpp"
#include "HAL/HAL.hpp"
#include "Fusion/Fusion.hpp"
#include "Fusion/FtpEstimator.hpp"
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

    // FTP estimation from ride data. See FtpEstimator for the protocol
    // (best 20-min avg x 0.95). Returns 0 if no qualifying window yet.
    uint16_t suggestedFtp() const { return _ftpEstimator.suggestedFtp(); }
    uint16_t best20MinAvg() const { return _ftpEstimator.best20MinAvg(); }
    bool ftpHasQualifyingWindow() const { return _ftpEstimator.hasQualifyingWindow(); }

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
    uint32_t _lastRenderMillis = 0;
    int messageType = 0;
    uint32_t _messageSendMillis = 0;
    bool startMessageConfig = false;

    bool _gpsEnableState = true;
    bool validLoc_prev = false;
    bool _prevGpsPosValid = false;  // frame.gpsPos.valid from the previous tick (resync edge-detect)

    uint8_t lastSecond;

    // Auto-pause state
    bool _autoPaused = false;      // paused by us (vs manually) - only then auto-resume
    uint32_t _stopSinceMs = 0;     // timestamp the speed first dropped below threshold
    uint32_t _moveSinceMs = 0;     // timestamp movement was first detected while auto-paused

    DataModel model;
    UIManager ui;
    FusionEngine _fusion;
    FtpEstimator _ftpEstimator;
    uint32_t _millis, _last_millis, lastGPS;

    void saveBiometrics();
    void loadBiometrics();

    void saveBikeStats();
    void loadBikeStats();

    // Auto-pause: stop logging automatically when the rider stops, resume
    // when they get moving again. Speed-based with hysteresis (pause below
    // 3 km/h, resume above 5 km/h) and time debounce so traffic lights do
    // not trigger it but real stops do. Cadence confirms movement on resume
    // (track-stand / rollout with no wheel-sensor speed yet).
    static constexpr float AUTO_PAUSE_SPEED_KMH   = 3.0f;
    static constexpr float AUTO_RESUME_SPEED_KMH  = 5.0f;
    static constexpr float AUTO_RESUME_CADENCE_RPM = 20.0f;
    static constexpr uint32_t AUTO_PAUSE_DELAY_MS  = 10000;
    static constexpr uint32_t AUTO_RESUME_DELAY_MS = 3000;
    void updateAutoPause(uint32_t now);

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
#ifndef APP_H
#define APP_H

#include "ui/UIManager.hpp"
#include "HAL/HAL.hpp"
#include "AppEvents.hpp"
#include "DataModel/DataModel.hpp"
#include "HAL/BluetoothInterface.hpp"
#include "App/SettingsStore.hpp"
#include "App/InputCallbacks.hpp"
#include "App/TelemetryPublisher.hpp"
#include "App/RideController.hpp"

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

    // Getters for UI access
    AppState getState() const { return state; }
    bool getGpsEnableState() const { return _inputCallbacks.gpsEnableState(); }

    bool isLogging() const { return state == AppState::LOGGING; }

    // FTP estimation from ride data. See FtpEstimator for the protocol
    // (best 20-min avg x 0.95). Returns 0 if no qualifying window yet.
    uint16_t suggestedFtp() const { return _ride.suggestedFtp(); }
    uint16_t best20MinAvg() const { return _ride.best20MinAvg(); }
    bool ftpHasQualifyingWindow() const { return _ride.ftpHasQualifyingWindow(); }

    void handleAppEvent(const AppEvent& e);
    
    const DataModel& getModel() const { return model; }
    DataModel& getModel() { return model; }

private:
    App() : model(), ui(*this) {}
    
    std::queue<AppEvent> appEvents;
    DataModel model;
    UIManager ui;
    SettingsStore _settings;
    InputCallbacks _inputCallbacks;
    TelemetryPublisher _telemetry;
    RideController _ride;

    AppState state = AppState::BOOT, state_prev = AppState::BOOT;
    uint32_t _lastRenderMillis = 0;
    bool _gpsSettingsLoaded = false;
    bool _gpsRateSyncStarted = false;
    // Idle auto-sleep state
    uint32_t _lastActivityMs = 0;  // timestamp of the last button press/hold
    bool _idleActive = false;      // true while in IDLE, re-arms the countdown on entry

    // Battery: debounced charging state + one-shot low-battery alarm.
    bool _chargingRaw = false;        // latest (undebounced) charger flag
    bool _chargingState = false;      // debounced published state
    uint8_t _chargingDebounce = 0;    // consecutive samples disagreeing
    bool _battLowLatched = false;     // low-battery beeped; until recharged

    // SD-card-removed alarm: one beep per removal while logging.
    bool _sdAlarmLatched = false;

    uint32_t _millis, _last_millis, lastGPS;

    // Beeper state (see buzz()/updateBuzzer()).
    std::vector<uint16_t> _buzzSteps;
    uint8_t _buzzStep = 0;
    bool _buzzActive = false;
    uint32_t _buzzStepEndMs = 0;

    // Buzzer feedback: non-blocking pattern player. Patterns alternate
    // on/off starting with ON; durations in ms. Driven from update().
    void buzz(const uint16_t* patternMs, uint8_t len);
    void updateBuzzer(uint32_t now);

    // Default idle-sleep timeout when the stored setting is 0 (see
    // BikeData::idleSleepMinutes).
    static constexpr uint8_t IDLE_SLEEP_DEFAULT_MIN = 5;
    void updateIdleSleep(const physIO& io, uint32_t now);

    // Ride timing / alarms / auto-lap (see App.cpp for details).
    void updateBatteryAlarm();
    void updateSdAlarm();
    void updateAutoLap();
    static uint8_t vbatToPct(float volts);

};

#endif /* APP_H */
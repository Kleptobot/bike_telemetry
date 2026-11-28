#include <Arduino.h>
#include <variant>
#include <cmath>
#include "App.hpp"

#include "ui/Screens/MainScreen.hpp"
#include "ui/Screens/BluetoothScreen.hpp"
#include "ui/Screens/TimeEditScreen.hpp"
#include "ui/Screens/SettingsScreen.hpp"
#include "ui/Screens/BiometricsScreen.hpp"
#include "ui/Screens/DisplayEditScreen.hpp"
#include "ui/Screens/GPSScreen.hpp"
#include "ui/Screens/UnmountSDScreen.hpp"
#include "ui/Screens/BikeStatsScreen.hpp"
#include "ui/Screens/LogListScreen.hpp"
#include "ui/Screens/LogSummaryScreen.hpp"

// ---- Buzzer feedback patterns (ms, alternating on/off starting with ON) ----
// Distinct feel per event: start = one short chirp, pause = one long beep,
// resume = double beep, stop = rising triple.
static constexpr uint16_t BUZZ_START[]  = {120};
static constexpr uint16_t BUZZ_PAUSE[]  = {350};
static constexpr uint16_t BUZZ_RESUME[] = {100, 100, 100};
static constexpr uint16_t BUZZ_STOP[]   = {100, 100, 100, 100, 300};
static constexpr uint16_t BUZZ_LAP[]      = {80, 80, 80};
static constexpr uint16_t BUZZ_BATT_LOW[] = {300, 200, 300};
static constexpr uint16_t BUZZ_SD_REMOVED[] = {250, 150, 250, 150, 250};

void App::buzz(const uint16_t* patternMs, uint8_t len) {
    if (len == 0) return;
    _buzzSteps.assign(patternMs, patternMs + len);
    _buzzStep = 0;
    _buzzActive = true;
    HAL::inst().buzzStart();
    _buzzStepEndMs = millis() + _buzzSteps[0];
}

void App::updateBuzzer(uint32_t now) {
    if (!_buzzActive) return;
    if ((int32_t)(now - _buzzStepEndMs) < 0) return;

    // Even steps are ON, odd steps are OFF.
    if ((_buzzStep % 2) == 0) HAL::inst().buzzStop();
    ++_buzzStep;
    if (_buzzStep >= _buzzSteps.size()) {
        _buzzActive = false;
        return;
    }
    if ((_buzzStep % 2) == 0) HAL::inst().buzzStart();
    _buzzStepEndMs = now + _buzzSteps[_buzzStep];
}

// ---------------------------------------------------------------------------
// Battery alarm: debounced charging state published to the bus, plus a
// one-shot low-battery beep (unlatches when charging or recharged).
// ---------------------------------------------------------------------------
uint8_t App::vbatToPct(float volts) {
    // Li-ion open-circuit-voltage curve, piecewise linear. Only meaningful
    // at rest; the caller suppresses it while charging.
    static constexpr float OCV[][2] = {
        {4.20f, 100.0f}, {4.10f, 90.0f}, {4.00f, 80.0f}, {3.90f, 65.0f},
        {3.80f, 50.0f},  {3.70f, 30.0f}, {3.65f, 20.0f}, {3.55f, 10.0f},
        {3.45f,  5.0f},  {3.30f,  0.0f},
    };
    constexpr size_t N = sizeof(OCV) / sizeof(OCV[0]);
    if (volts <= 0.0f) return 0;
    if (volts >= OCV[0][0]) return 100;
    for (size_t i = 0; i + 1 < N; i++) {
        if (volts >= OCV[i + 1][0]) {
            const float frac = (volts - OCV[i][0]) / (OCV[i + 1][0] - OCV[i][0]);
            return (uint8_t)constrain(OCV[i][1] + frac * (OCV[i + 1][1] - OCV[i][1]), 0.0f, 100.0f);
        }
    }
    return 0;
}

void App::updateBatteryAlarm() {
    const auto& f = HAL::inst().measurements();

    // Debounce the charger flag: 2 consecutive agreeing samples commit.
    const bool raw = f.charging.valid && f.charging.value;
    if (raw != _chargingRaw) {
        if (++_chargingDebounce >= 2) {
            _chargingRaw = raw;
            _chargingDebounce = 0;
            if (_chargingState != _chargingRaw) {
                _chargingState = _chargingRaw;
                if (_chargingState) _battLowLatched = false;   // on charger: alarm off
            }
        }
    } else {
        _chargingDebounce = 0;
    }
    model.bus().publish(Topic::BatteryCharging, _chargingState);

    const uint8_t pct = vbatToPct(f.vbatVolts.value);
    if (!_battLowLatched && !_chargingState && pct > 0 && pct < 15) {
        _battLowLatched = true;
        Serial.println("[App] battery low");
        buzz(BUZZ_BATT_LOW, sizeof(BUZZ_BATT_LOW) / sizeof(BUZZ_BATT_LOW[0]));
    }
    if (_battLowLatched && (_chargingState || pct > 25)) {
        _battLowLatched = false;
    }
}

// ---------------------------------------------------------------------------
// SD-card alarm: single beep pattern per removal while a ride is active.
// Recovery/remount stays with the existing SD flow.
// ---------------------------------------------------------------------------
void App::updateSdAlarm() {
    const bool present = HAL::inst().measurements().sdPresent;
    if (!present) {
        if (!_sdAlarmLatched &&
            (state == AppState::LOGGING || state == AppState::PAUSED)) {
            _sdAlarmLatched = true;
            Serial.println("[App] SD card removed while logging!");
            buzz(BUZZ_SD_REMOVED, sizeof(BUZZ_SD_REMOVED) / sizeof(BUZZ_SD_REMOVED[0]));
        }
    } else {
        _sdAlarmLatched = false;
    }
}

// ---------------------------------------------------------------------------
// Auto-lap: fire NewLap when the configured distance or moving-time increment
// has elapsed since the previous lap. Only while actively logging.
// ---------------------------------------------------------------------------
void App::updateAutoLap() {
    if (_ride.shouldAutoLap(state)) {
        postAppEvent({AppEventType::NewLap, 0});
        buzz(BUZZ_LAP, sizeof(BUZZ_LAP) / sizeof(BUZZ_LAP[0]));
        Serial.println("[App] auto-lap fired");
    }
}

void App::begin(IStorage* storage) {
    _settings.begin(storage, &model);
    _telemetry.begin(model);
    _ride.begin(storage, model, _telemetry, _settings);

    state = AppState::BOOT;
    Disp::init();

    ui.registerScreen<MainScreen>(ScreenID::MainMenu,App::instance().getModel());
    ui.registerScreen<TimeEditScreen>(ScreenID::TimeMenu,App::instance().getModel());
    ui.registerScreen<SettingsScreen>(ScreenID::SettingsMenu,App::instance().getModel());
    ui.registerScreen<BluetoothScreen>(ScreenID::Bluetooth,App::instance().getModel());
    ui.registerScreen<BiometricsScreen>(ScreenID::Biometrics,App::instance().getModel());
    ui.registerScreen<DisplayEditScreen>(ScreenID::DisplayEdit,App::instance().getModel());
    ui.registerScreen<GPSScreen>(ScreenID::GPSSettings,App::instance().getModel());
    ui.registerScreen<UnmountSDScreen>(ScreenID::UnmountSD,App::instance().getModel());
    ui.registerScreen<BikeStatsScreen>(ScreenID::BikeStats,App::instance().getModel());
    ui.registerScreen<LogListScreen>(ScreenID::LogList,App::instance().getModel());
    ui.registerScreen<LogSummaryScreen>(ScreenID::LogSummary,App::instance().getModel());

    ui.begin(ScreenID::MainMenu);

    _inputCallbacks.begin(model);

    Serial.println("[App] Started and registered telemetry callback.");
}

void App::update() {
    _telemetry.update();
    // Any periodic application-level behavior here
    const DataBus& bus = model.bus();

    const bool sdMounted = HAL::inst().SDMounted();
    model.SD().update({sdMounted, !HAL::inst().inputs().SD_Det.state});
    if (sdMounted && !_sdWasMounted && state != AppState::BOOT) {
        _sdSettingsReloadPending = true;
    }
    if (!sdMounted) {
        _sdSettingsReloadPending = false;
    }
    if (_sdSettingsReloadPending && state == AppState::IDLE) {
        Serial.println("[App] SD card mounted; reloading settings.");
        _sdSettingsReloadPending = false;
        state = AppState::BOOT;
    }
    _sdWasMounted = sdMounted;

    if (!appEvents.empty()) {
        handleAppEvent(appEvents.front());
        appEvents.pop();
    }
    _millis = millis();

    //only render the UI every 100ms to save some CPU, but update and handle inputs every loop
    if (_millis - _lastRenderMillis > 100) {
        _lastRenderMillis = _millis;
        ui.render();
    }

    ui.update((float)(_millis - _last_millis) / 1000.0);
    const physIO io = HAL::inst().inputs();
    ui.handleInput(io);

    const AutoPauseAction autoPauseAction = _ride.updateAutoPause(state, _millis);
    if (autoPauseAction == AutoPauseAction::Pause) {
        postAppEvent({AppEventType::PauseLogging, 0});
        Serial.println("[App] auto-paused (stopped)");
    } else if (autoPauseAction == AutoPauseAction::Resume) {
        postAppEvent({AppEventType::ResumeLogging, 0});
        Serial.println("[App] auto-resumed (moving)");
    }
    updateIdleSleep(io, _millis);
    updateBuzzer(_millis);
    _ride.updateTimers((float)(_millis - _last_millis) / 1000.0f, state);
    updateBatteryAlarm();
    updateSdAlarm();
    updateAutoLap();
    _last_millis = _millis;

    const bool gpsValid = bus.get<bool>(Topic::GpsValid);
    if (gpsValid && _gpsSettingsLoaded && !_gpsRateSyncStarted) {
        HAL::inst().reconcileGpsNmeaRates(
            model.gpsSettings().get().nmeaRates, false);
        _gpsRateSyncStarted = true;
    }

    const AppState rideState = state;
    switch(state) {
        case AppState::BOOT:
            Serial.println("Loading data from filesystem.");
            _gpsSettingsLoaded = false;
            _gpsRateSyncStarted = false;
            HAL::inst().bluetooth().loadDevices();
            HAL::inst().resetGPS();
            _settings.loadBiometrics();
            _settings.loadBikeStats();
            _settings.loadLayout();
            _settings.loadTime();
            _settings.loadGpsSettings();
            _gpsSettingsLoaded = true;

            _ride.configureLogger();

            ui.showScreen(ScreenID::MainMenu);
            state = AppState::IDLE;
            break;

        case AppState::IDLE:
            _ride.update(rideState, state_prev);
            if (!HAL::inst().bluetooth().all_devices_discovered())
                HAL::inst().bluetooth().setMode(E_Type_BT_Mode::connect);
            else
                HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);

            //on SD card detection go back to boot
            if (HAL::inst().inputs().SD_Det.FE) {
                HAL::inst().reInitStorage(&model.time().get() );
                state = AppState::BOOT;
            } else if (HAL::inst().inputs().SD_Det.RE) {
                //SD card removed
            }

            break;

        case AppState::CONFIG:
            HAL::inst().bluetooth().setMode(E_Type_BT_Mode::scan);
            break;

        case AppState::LOGGING:
            HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);
            if (!_ride.update(rideState, state_prev)) state = AppState::IDLE;
            break;
        
        case AppState::PAUSED:
            _ride.update(rideState, state_prev);
            break;
        
        default:
            break;

    }
    if (rideState != AppState::IDLE && rideState != AppState::LOGGING &&
        rideState != AppState::PAUSED) {
        _ride.update(rideState, state_prev);
    }
    state_prev = state;
    model.setAppState(state);
}

void App::updateIdleSleep(const physIO& io, uint32_t now) {
    // Any button physically down counts as user activity - it covers short
    // presses, long holds and the sleep gesture without edge-case misses.
    const bool activity = io.Up.state || io.Down.state || io.Left.state ||
                          io.Right.state || io.Select.state;
    if (activity) {
        _lastActivityMs = now;
        return;
    }

    // Arm the countdown on IDLE entry, disarm when leaving it. Re-entering
    // IDLE (e.g. after finishing a ride) restarts the countdown.
    if (state == AppState::IDLE && !_idleActive) {
        _idleActive = true;
        _lastActivityMs = now;
    } else if (state != AppState::IDLE) {
        _idleActive = false;
        return;
    }

    // A stored 0 is clamped so the setting can never mean instant sleep.
    const uint8_t minutes = model.bike().get().idleSleepMinutes
                                ? model.bike().get().idleSleepMinutes
                                : IDLE_SLEEP_DEFAULT_MIN;
    const uint32_t timeoutMs = (uint32_t)minutes * 60000UL;

    if (now - _lastActivityMs > timeoutMs) {
        Serial.println("[App] idle timeout, sleeping");
        postAppEvent({AppEventType::Sleep,0});
        // Keep _idleActive armed and _lastActivityMs old: until system-off
        // actually happens, _lastActivityMs will only reset on new input, so
        // this branch fires at most once more (harmless: sleep is idempotent).
    }
}

void App::handleAppEvent(const AppEvent& e) {
    switch (e.type) {
        case AppEventType::SaveTime:
            HAL::inst().setTime(std::get<timeData>(e.payload).utcDateTime());
            _settings.saveTime();
            break;

        case AppEventType::SaveBiometrics:
            _settings.saveBiometrics();
            break;

        case AppEventType::SaveLayout:
            _settings.saveLayout();
            break;

        case AppEventType::StartLogging:
            HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);
            _ride.start(state);
            buzz(BUZZ_START, sizeof(BUZZ_START) / sizeof(BUZZ_START[0]));
            break;

        case AppEventType::StopLogging:
            _ride.stop(state);
            buzz(BUZZ_STOP, sizeof(BUZZ_STOP) / sizeof(BUZZ_STOP[0]));
            break;

        case AppEventType::PauseLogging:
            _ride.pause(state);
            if (state == AppState::PAUSED)
                buzz(BUZZ_PAUSE, sizeof(BUZZ_PAUSE) / sizeof(BUZZ_PAUSE[0]));
            break;

        case AppEventType::ResumeLogging:
            _ride.resume(state);
            if (state == AppState::LOGGING)
                buzz(BUZZ_RESUME, sizeof(BUZZ_RESUME) / sizeof(BUZZ_RESUME[0]));
            break;

        case AppEventType::NewLap:
            if (!_ride.newLap(state)) state = AppState::IDLE;
            break;

        case AppEventType::ConnectBluetooth:
            HAL::inst().bluetooth().createDevice(std::get<BluetoothDevice>(e.payload));
            break;

        case AppEventType::DisconnectBluetooth:
            HAL::inst().bluetooth().disconnectDevice(std::get<BluetoothDevice>(e.payload));
            break;

        case AppEventType::ScanBluetooth:
            HAL::inst().bluetooth().saveDevices();
            state = AppState::IDLE;
            break;

        case AppEventType::DiscoverBluetooth:
            state = AppState::CONFIG;
            break;

        case AppEventType::Sleep:
            HAL::inst().sleep();
            break;

        case AppEventType::RestoreDefaultsGPS:
            HAL::inst().gpsRestoreDefaults();
            break;

        case AppEventType::ResetGPS:
            HAL::inst().resetGPS();
            break;

        case AppEventType::SaveGPSConfig: {
            const GpsSettings& settings = std::get<GpsSettings>(e.payload);
            if (!_settings.saveGpsSettings(settings)) {
                Serial.println("[App] GPS configuration was not saved; device sync skipped.");
                break;
            }
            model.gpsSettings().update(settings);
            HAL::inst().reconcileGpsNmeaRates(settings.nmeaRates, true);
            break;
        }

        case AppEventType::UnmountSD:
            HAL::inst().unMountSD();
            break;

        case AppEventType::SaveBikeStats:
            _settings.saveBikeStats();
            _ride.configureLogger();
            break;

        default:
            break;
    }
}

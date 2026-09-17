#include <Arduino.h>
#include <variant>
#include <cmath>
#include "App.hpp"

#include "HAL/SDCard.hpp"

#include "UI/Screens/MainScreen.hpp"
#include "UI/Screens/BluetoothScreen.hpp"
#include "UI/Screens/TimeEditScreen.hpp"
#include "UI/Screens/SettingsScreen.hpp"
#include "UI/Screens/BiometricsScreen.hpp"
#include "UI/Screens/DisplayEditScreen.hpp"
#include "UI/Screens/GPSScreen.hpp"
#include "UI/Screens/UnmountSDScreen.hpp"
#include "UI/Screens/BikeStatsScreen.hpp"
#include "UI/Screens/LogListScreen.hpp"
#include "UI/Screens/LogSummaryScreen.hpp"

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
// Ride timers: elapsed (logging active, pauses included) and moving
// (LOGGING only, pauses excluded). Published to the bus every tick.
// ---------------------------------------------------------------------------
void App::updateRideTimers(float dt) {
    if (dt > 0.0f && dt < 5.0f) {
        if (state == AppState::LOGGING || state == AppState::PAUSED) {
            _rideElapsedSec += dt;
            if (state == AppState::LOGGING) _rideMovingSec += dt;
        }
    }
    DataBus& bus = model.bus();
    bus.publish(Topic::ElapsedSec, _rideElapsedSec);
    bus.publish(Topic::MovingSec,  _rideMovingSec);
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
    const auto& bike = model.bike().get();
    if (bike.autoLapMode == AutoLapMode::Off) return;
    if (state != AppState::LOGGING) return;

    bool fire = false;
    if (bike.autoLapMode == AutoLapMode::Distance) {
        if (bike.autoLapValue == 0) return;
        const float dist = model.bus().get<float>(Topic::TotalDistanceM);
        if (dist - _lastLapDistanceM >= (float)bike.autoLapValue * 1000.0f) fire = true;
    } else {   // Time
        if (bike.autoLapValue == 0) return;
        if (_rideMovingSec - _lastLapMovingSec >= (float)bike.autoLapValue * 60.0f) fire = true;
    }

    if (fire) {
        _lastLapDistanceM = model.bus().get<float>(Topic::TotalDistanceM);
        _lastLapMovingSec = _rideMovingSec;
        postAppEvent({AppEventType::NewLap, 0});
        buzz(BUZZ_LAP, sizeof(BUZZ_LAP) / sizeof(BUZZ_LAP[0]));
        Serial.println("[App] auto-lap fired");
    }
}

void App::begin(IStorage* storage) {
    _storage = storage;

    // The storage interface *is* the SDCardSystem -- wire it up for USB MSC.
    // Safe to static_cast: HAL::inst().SD() returns the SDCardSystem instance.
    _usbMsc.setSDCardSystem(static_cast<SDCardSystem*>(storage));

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

    HAL::inst().bluetooth().onDeviceList([this](std::vector<BluetoothDevice> devices) {
        this->updateBluetooth(devices);
    });

    InputSystem::onEnableStateRead([this](bool pinState) {
        this->updateGpsEnable(pinState);
    });

    Serial.println("[App] Started and registered telemetry callback.");
}

void App::update() {
    updateTelemetry();
    // Any periodic application-level behavior here
    const timeData& currentTime = model.time().get();
    const DataBus& bus = model.bus();

    model.SD().update ({HAL::inst().SDMounted(),!HAL::inst().inputs().SD_Det.state});

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

    // USB MSC must never run concurrently with the firmware's own filesystem
    // access: the Logs/LogSummary screens call SdFat on the main thread while
    // the USB task would be doing raw block I/O on the same card. Menu
    // navigation does not change AppState (it stays IDLE), so gate on the
    // active screen: MSC runs only while the idle home screen is shown.
    if (state == AppState::IDLE && ui.currentScreen() == ScreenID::MainMenu) {
        if (!_usbMsc.isEnabled()) _usbMsc.resume();
    } else if (_usbMsc.isEnabled()) {
        _usbMsc.pause();
    }

    updateAutoPause(_millis);
    updateIdleSleep(io, _millis);
    updateBuzzer(_millis);
    updateRideTimers((float)(_millis - _last_millis) / 1000.0f);
    updateBatteryAlarm();
    updateSdAlarm();
    updateAutoLap();
    _last_millis = _millis;

    const bool gpsValid = bus.get<bool>(Topic::GpsValid);
    if (gpsValid && ! validLoc_prev){
        startMessageConfig = true;

    }
    validLoc_prev = gpsValid;
    if (startMessageConfig){
        switch (messageType)
        {
        case 0: case 4: 
             HAL::inst().setNMEArates(messageType,1);
             messageType+=10;
             _messageSendMillis = _millis;
            break;

        case 1: case 2: case 3: case 5: case 6: case 7: case 8: case 9:
             HAL::inst().setNMEArates(messageType,0);
             messageType+=10;
             _messageSendMillis = _millis;
            break;

        case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18:
            //wait here for 200ms
            if (_millis - _messageSendMillis > 200) {
                _lastRenderMillis = _millis;
                messageType-=9;
            }
            break;

        case 19:
            HAL::inst().setRMCRate();
            startMessageConfig = false;
            break;
        
        default:
            break;
        }
    }

    switch(state) {
        case AppState::BOOT:
            Serial.println("Loading data from filesystem.");
            HAL::inst().bluetooth().loadDevices();
            HAL::inst().resetGPS();
            loadBiometrics();
            loadBikeStats();
            loadLayout();
            loadTime();

            updateLoggerInstance();

            ui.showScreen(ScreenID::MainMenu);
            state = AppState::IDLE;
            break;

        case AppState::IDLE:
            // Expose the SD card as a USB mass storage device so the host
            // can read/write log files. Only safe here: no logger is active,
            // so the filesystem is not being mutated by the firmware.
            _usbMsc.begin();

            if (!HAL::inst().bluetooth().all_devices_discovered())
                HAL::inst().bluetooth().setMode(E_Type_BT_Mode::connect);
            else
                HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);
            if ((state_prev == AppState::LOGGING || state_prev == AppState::PAUSED) && _logger) {
                Serial.println("[App] Finalizing logger.");
                _logger->finaliseLogging();

                // Passive FTP update: if this ride produced a qualifying
                // 20-minute window and beat the stored FTP, auto-save the
                // new max to biometrics. User can always override manually.
                if (_ftpEstimator.hasQualifyingWindow()) {
                    uint16_t rideFtp = _ftpEstimator.suggestedFtp();
                    if (rideFtp > model.bio().get().ftpWatts) {
                        BioData updated = model.bio().get();
                        updated.ftpWatts = rideFtp;
                        model.bio().update(updated);
                        saveBiometrics();
                        Serial.print("[App] FTP PB updated to ");
                        Serial.println(rideFtp);
                    }
                }
            }
            _fusion.resetDistance();

            //on SD card detection go back to boot
            if (HAL::inst().inputs().SD_Det.FE) {
                HAL::inst().reInitStorage(&model.time().get() );
                state = AppState::BOOT;
            } else if (HAL::inst().inputs().SD_Det.RE) {
                //SD card removed
            }

            break;

        case AppState::CONFIG:
            _usbMsc.end();
            HAL::inst().bluetooth().setMode(E_Type_BT_Mode::scan);
            break;

        case AppState::LOGGING:
            _usbMsc.end();
            HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);

            if (state_prev != AppState::LOGGING && state_prev != AppState::PAUSED) {
                _logger->startLogging(currentTime);
                _fusion.resetDistance();
                _ftpEstimator.reset();
            }

            model.logger().update({_logger->elapsed_Total(),_logger->elapsed_Lap()});
            
            //check if seconds has changed for logging tick
            if (currentTime.second() != lastSecond) {
                // Assemble the trackpoint from the bus -- the same published
                // channels the UI tiles show, so logs can never disagree with
                // the display. (Was a Telemetry struct copy.)
                const location_data& loc = bus.get<location_data>(Topic::Location);
                Trackpoint tp;
                                tp.latitude  = loc.latitude;
                tp.longitude = loc.longitude;
                tp.altitude  = bus.get<float>(Topic::FusedAltitude);
                tp.speed     = bus.get<float>(Topic::SelectedSpeed);
                tp.heartrate = bus.get<float>(Topic::HeartRate);
                tp.power     = bus.get<float>(Topic::PowerMeter);
                tp.cadence   = bus.get<float>(Topic::Cadence);
                tp.distance  = bus.get<float>(Topic::TotalDistanceM);
                // Fitness channels (Phase B: wire live FitnessFusion values into the trackpoint)
                tp.calories          = bus.get<float>(Topic::Calories);
                tp.normalizedPower   = bus.get<float>(Topic::NormalizedPower);
                tp.intensityFactor   = bus.get<float>(Topic::IntensityFactor);
                tp.tss               = bus.get<float>(Topic::Tss);
                tp.hrZone            = bus.get<uint8_t>(Topic::HrZone);
                tp.powerZone         = bus.get<uint8_t>(Topic::PowerZone);
                _logger->addTrackpoint(tp, currentTime);

                // Feed the FTP estimator: best available power source (meter
                // if live, otherwise the physics estimate) at 1 Hz.
                const float powerWatts = bus.get<float>(Topic::PowerMeter) > 0.0f
                    ? bus.get<float>(Topic::PowerMeter)
                    : bus.get<float>(Topic::EstimatedPower);
                _ftpEstimator.addSample((uint16_t)(powerWatts > 0.0f ? powerWatts + 0.5f : 0.0f));
            }
            break;
        
        case AppState::PAUSED:
            _usbMsc.end();
            break;
        
        default:
            _usbMsc.end();
            break;

    }
    lastSecond = currentTime.second();
    state_prev = state;
    model.setAppState(state);
}

void App::updateTelemetry() {
    // Run the fusion pipeline over this tick's acquisition frame. All the
    // derivation that used to live here (Haversine distance, speed-source
    // selection, grade) and inside HAL (baro/GPS/IMU altitude fusion) is now
    // in one place, fed by timestamped samples.
    const MeasurementFrame& frame = HAL::inst().measurements();

    // Snapshot the rider/bike settings into the profile the fusion pipeline
    // consumes. Cheap value semantics; the source of truth stays in DataModel.
    const auto& bio  = model.bio().get();
    const auto& bike = model.bike().get();
    FitnessProfile profile;
    profile.riderMassKg      = (float)bio.mass;
    profile.bikeMassKg       = bike.mass / 10.0f;      // stored in tenths of kg
    profile.ageYears         = timeDuration(model.time().get() - bio.birthday).years();
    profile.caloricProfile   = bio.caloricProfile;
    profile.zoneStartsBpm[0] = (float)bio.zone1Start;
    profile.zoneStartsBpm[1] = (float)bio.zone2Start;
    profile.zoneStartsBpm[2] = (float)bio.zone3Start;
    profile.zoneStartsBpm[3] = (float)bio.zone4Start;
    profile.zoneStartsBpm[4] = (float)bio.zone5Start;
    profile.ftpWatts         = bio.ftpWatts;

    _fusion.update(frame, bike.wheelCircumference, profile);
    const DerivedChannels& d = _fusion.out();

    // --- Publish fusion outputs to the DataBus -------------------------------
    // Subscribers (loggers, UI widgets, analytics) are notified synchronously.
    // Polling access is also available via model.bus().get<T>(Topic::X).
    DataBus& bus = model.bus();
    bus.publish(Topic::FusedAltitude, d.altitudeM);
    bus.publish(Topic::BaroAlt, d.baroAltitudeM);
    bus.publish(Topic::Vario, d.varioMs);
    bus.publish(Topic::SelectedSpeed, d.speedKmh);
    bus.publish(Topic::SmoothedGrade, d.gradePct);
    bus.publish(Topic::Cadence, d.cadenceRpm);
    bus.publish(Topic::GearRatio, d.gearRatio);
    bus.publish(Topic::Energy, d.energyKj);
    bus.publish(Topic::Acceleration, d.accelMs2);
    bus.publish(Topic::EstimatedPower, d.estPowerWatts);
    bus.publish(Topic::TotalAscent, d.totalAscentM);
    bus.publish(Topic::TotalDescent, d.totalDescentM);
    bus.publish(Topic::Coasting, d.coasting);
    bus.publish(Topic::Calories, d.caloriesKcal);
    bus.publish(Topic::NormalizedPower, d.normalizedPowerW);
    bus.publish(Topic::IntensityFactor, d.intensityFactor);
    bus.publish(Topic::Tss, d.tss);
    bus.publish(Topic::HrZone, d.hrZone);
    bus.publish(Topic::PowerZone, d.powerZone);
    bus.publish(Topic::Avg3sPower,  d.avg3sPowerW);
    bus.publish(Topic::WattsPerKg,  d.wattsPerKg);
    bus.publish(Topic::Vam,         d.vamMph);
    bus.publish(Topic::GpsQuality,  d.gpsQuality);
    bus.publish(Topic::ShiftCount,  d.shiftCount);
    bus.publish(Topic::GearShifted, d.gearShifted);

    // --- Publish position/distance/system channels to the DataBus -------------
    // These replace the last duties of the Telemetry struct: the UI (battery
    // icon, GPS icon, Location/Distance tiles) and the loggers' Trackpoint
    // all read from the bus now. imu/dps raw blocks are not published -- no
    // consumer ever read them from Telemetry.
    const int16_t batteryPct = frame.batteryPct;
    location_data loc;
    loc.valid     = frame.gpsPos.valid;
    loc.longitude = frame.gpsPos.lng;
    loc.latitude  = frame.gpsPos.lat;

    bus.publish(Topic::GpsValid,       frame.gpsPos.valid);
    bus.publish(Topic::Location,       loc);
    bus.publish(Topic::DistanceDeltaM, d.distanceDeltaM);
    bus.publish(Topic::TotalDistanceM, d.totalDistanceM);
    bus.publish(Topic::Battery,        batteryPct);

    // BLE channels carry data_record (value + live). Publish the value when
    // the sensor is live, zero otherwise.
    bus.publish(Topic::HeartRate,  frame.heartRateBpm.live      ? frame.heartRateBpm.value      : 0.0f);
    bus.publish(Topic::PowerMeter, frame.powerWatts.live         ? frame.powerWatts.value         : 0.0f);
    bus.publish(Topic::Temperature, d.temperatureC);

    // --- Publish remaining measured channels ---------------------------------
    // GPS doubles narrow to float here: display precision, and it keeps the
    // bus uniform for the widget's float-based subscription callback.
    bus.publish(Topic::GpsSpeed,    frame.gpsSpeedKmh.valid ? (float)frame.gpsSpeedKmh.value : 0.0f);
    bus.publish(Topic::GpsAltitude, frame.gpsAltM.valid     ? (float)frame.gpsAltM.value     : 0.0f);
    bus.publish(Topic::GpsCourse,   frame.gpsCourseDeg.valid ? (float)frame.gpsCourseDeg.value : 0.0f);
    bus.publish(Topic::GpsSats,     frame.gpsSatsUsed.value);
    bus.publish(Topic::GpsHdop,     frame.gpsHdop.valid     ? (float)frame.gpsHdop.value     : 0.0f);

    // CPS decodes torque/balance/force but they were dropped at the frame
    // boundary until now; balance and torque are worth a tile.
    bus.publish(Topic::TorqueNm,     frame.torqueNm.live       ? frame.torqueNm.value       : 0.0f);
    bus.publish(Topic::PedalBalance, frame.pedalBalancePct.live ? frame.pedalBalancePct.value : 0.0f);

    bus.publish(Topic::BatteryVolts, frame.vbatVolts.value);

    // Breadcrumb trail for the map widget. The store decimates to 1 Hz and
    // rejects duplicate positions internally, so feeding every tick is fine.
    if (frame.gpsPos.valid) {
        model.gpsTrack().append(frame.gpsPos.lat, frame.gpsPos.lng, millis());
    }

    // --- GPS → RTC resync -----------------------------------------------------
    // Trigger on each fresh GPS position commit: frame.gpsPos.seq advancing
    // means a new fix has been parsed. The edge-detector (_prevGpsPosValid)
    // fires once per transition from no-fix-to-fix, matching the original
    // behaviour that tracked gpsLoc.isValid() across ticks.
    int UTCoffset = model.time().get().offset();
    const bool newFix = frame.gpsPos.valid && !_prevGpsPosValid;
    if (newFix && frame.gpsUtcTimeHMS.valid && frame.gpsUtcDateYMD.valid) {
        // Decode the frame's HHMMSS / YYYYMMDD encodings into a struct tm.
        const uint32_t hms = frame.gpsUtcTimeHMS.value;
        const uint32_t ymd = frame.gpsUtcDateYMD.value;
        struct tm gpsNow;
        gpsNow.tm_year = (ymd / 10000) - 1900;
        gpsNow.tm_mon  = ((ymd / 100) % 100) - 1;
        gpsNow.tm_mday =  ymd % 100;
        gpsNow.tm_hour =  hms / 10000;
        gpsNow.tm_min  = (hms / 100) % 100;
        gpsNow.tm_sec  =  hms % 100;
        gpsNow.tm_isdst = 0;

        time_t gpsEpoch = mktime(&gpsNow);
        time_t rtcEpoch = frame.rtcNow.value;
        time_t diff = difftime(gpsEpoch, rtcEpoch);
        if (diff < -30 || diff > 30) {
            HAL::inst().setTime(gpsNow);
        }
    }
    _prevGpsPosValid = frame.gpsPos.valid;

    model.time().update({frame.rtcNow.value, UTCoffset});
}

void App::updateBluetooth(std::vector<BluetoothDevice> devices) {
    model.bluetooth().update(devices);
}

void App::updateGpsEnable(bool state) {
    _gpsEnableState = state;
}

void App::updateAutoPause(uint32_t now) {
    if (!model.bike().get().autoPause) return;

    const float speedKmh = model.bus().get<float>(Topic::SelectedSpeed);
    const float cadence  = model.bus().get<float>(Topic::Cadence);
    const bool gpsValid  = model.bus().get<bool>(Topic::GpsValid);

    // No wheel sensor speed and no GPS fix means we have no trustworthy
    // speed source: never pause on absence of data.
    const bool speedUsable = gpsValid || speedKmh > 0.0f || cadence > 0.0f;
    if (!speedUsable) { _stopSinceMs = 0; _moveSinceMs = 0; return; }

    if (state == AppState::LOGGING && !_autoPaused) {
        if (speedKmh < AUTO_PAUSE_SPEED_KMH) {
            if (_stopSinceMs == 0) {
                _stopSinceMs = now;
            } else if (now - _stopSinceMs > AUTO_PAUSE_DELAY_MS) {
                _autoPaused = true;
                _stopSinceMs = 0;
                postAppEvent({AppEventType::PauseLogging,0});
                Serial.println("[App] auto-paused (stopped)");
            }
        } else {
            _stopSinceMs = 0;
        }
    } else if (state == AppState::PAUSED && _autoPaused) {
        if (speedKmh > AUTO_RESUME_SPEED_KMH || cadence > AUTO_RESUME_CADENCE_RPM) {
            if (_moveSinceMs == 0) {
                _moveSinceMs = now;
            } else if (now - _moveSinceMs > AUTO_RESUME_DELAY_MS) {
                _autoPaused = false;
                _moveSinceMs = 0;
                postAppEvent({AppEventType::ResumeLogging,0});
                Serial.println("[App] auto-resumed (moving)");
            }
        } else {
            _moveSinceMs = 0;
        }
    } else {
        // Manual pause/resume happened (or we are not logging): reset the
        // debounce timers. Manual resume leaves state==LOGGING with
        // _autoPaused still set - clear it so only auto-pauses auto-resume.
        _stopSinceMs = 0;
        _moveSinceMs = 0;
        if (state == AppState::LOGGING) _autoPaused = false;
    }
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
            saveTime();
            break;

        case AppEventType::SaveBiometrics:
            saveBiometrics();
            break;

        case AppEventType::SaveLayout:
            saveLayout();
            break;

        case AppEventType::StartLogging:
            HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);
            state = AppState::LOGGING;
            // New ride: reset timers and auto-lap anchors (distance/fitness
            // accumulators reset via FusionEngine::resetDistance() in update).
            _rideElapsedSec = 0.0f;
            _rideMovingSec  = 0.0f;
            _lastLapDistanceM = 0.0f;
            _lastLapMovingSec = 0.0f;
            buzz(BUZZ_START, sizeof(BUZZ_START) / sizeof(BUZZ_START[0]));
            break;

        case AppEventType::StopLogging:
            state = AppState::IDLE;
            buzz(BUZZ_STOP, sizeof(BUZZ_STOP) / sizeof(BUZZ_STOP[0]));
            break;

        case AppEventType::PauseLogging:
            if (state == AppState::LOGGING && _logger) {
                _logger->pause(model.time().get());
                state = AppState::PAUSED;
                buzz(BUZZ_PAUSE, sizeof(BUZZ_PAUSE) / sizeof(BUZZ_PAUSE[0]));
            }
            break;

        case AppEventType::ResumeLogging:
            if (state == AppState::PAUSED) {
                if (_logger) _logger->resume(model.time().get());
                state = AppState::LOGGING;
                buzz(BUZZ_RESUME, sizeof(BUZZ_RESUME) / sizeof(BUZZ_RESUME[0]));
            }
            break;

        case AppEventType::NewLap:
            if (_logger && (state == AppState::LOGGING || state == AppState::PAUSED)) {
                _logger->newLap(model.time().get());
                Serial.println("[App] New lap.");
            }
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

        case AppEventType::saveGPSNVRAM:
            HAL::inst().gpsSaveNVRAM();
            break;

        case AppEventType::setGPSNMEARate: {
                NMEArateChange change = std::get<NMEArateChange>(e.payload);
                HAL::inst().setNMEArates(change.type, change.rate);
            }
            break;

        case AppEventType::UnmountSD:
            HAL::inst().unMountSD();
            break;

        case AppEventType::SaveBikeStats:
            saveBikeStats();
            updateLoggerInstance();
            break;

        default:
            break;
    }
}
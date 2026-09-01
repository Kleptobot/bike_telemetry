#include <Arduino.h>
#include <variant>
#include <cmath>
#include "App.hpp"

#include "UI/Screens/MainScreen.hpp"
#include "UI/Screens/BluetoothScreen.hpp"
#include "UI/Screens/TimeEditScreen.hpp"
#include "UI/Screens/SettingsScreen.hpp"
#include "UI/Screens/BiometricsScreen.hpp"
#include "UI/Screens/DisplayEditScreen.hpp"
#include "UI/Screens/GPSScreen.hpp"
#include "UI/Screens/UnmountSDScreen.hpp"
#include "UI/Screens/BikeStatsScreen.hpp"

void App::begin(IStorage* storage) {
    _storage = storage;

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
    ui.handleInput(HAL::inst().inputs());
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
            if (!HAL::inst().bluetooth().all_devices_discovered())
                HAL::inst().bluetooth().setMode(E_Type_BT_Mode::connect);
            else
                HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);
            if (state_prev == AppState::LOGGING && _logger) {
                Serial.println("[App] Finalizing logger.");
                _logger->finaliseLogging();
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
            HAL::inst().bluetooth().setMode(E_Type_BT_Mode::scan);
            break;

        case AppState::LOGGING:
            HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);

            if(state != state_prev) {
                _logger->startLogging(currentTime);
                _fusion.resetDistance();
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
                _logger->addTrackpoint(tp, currentTime);
            }
            break;
        
        case AppState::PAUSED:
            break;
        
        default:
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
    _fusion.update(frame, model.bike().get().wheelCircumference);
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
            break;

        case AppEventType::StopLogging:
            state = AppState::IDLE;
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

void App::saveBiometrics() {
    JsonDocument doc;

    auto& a = model.bio().get();
    
    doc["birthday"] = a.birthday.unixtime();
    doc["mass"] = a.mass;
    doc["caloricProfile"] = toString(a.caloricProfile);
    doc["zone1Start"] = a.zone1Start;
    doc["zone2Start"] = a.zone2Start;
    doc["zone3Start"] = a.zone3Start;
    doc["zone4Start"] = a.zone4Start;
    doc["zone5Start"] = a.zone5Start;

    if (_storage->exists("/biometrics.txt"))
        _storage->remove("/biometrics.txt");

    File32 dataFile = _storage->openFile("/biometrics.txt", FILE_WRITE);
    if (!dataFile) {
        Serial.println("[App] failed to open /biometrics.txt for writing");
        return;
    }

    serializeJson(doc, dataFile);
    dataFile.close();
}

void App::loadBiometrics() {
    
    if (_storage->exists("/biometrics.txt")) {
        Serial.println("Found biometrics.txt");
        // Open file for reading
        File32 dataFile = _storage->openFile("/biometrics.txt", FILE_READ);
        if (!dataFile) {
            Serial.println("[App] failed to open /biometrics.txt for reading");
            return;
        }
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("[App] deserializeJson() failed for /biometrics.txt: ");
            Serial.println(error.c_str());
            dataFile.close();
            return;
        }

        BioData a;
        long long unix = jsonBuffer["birthday"];
        timeData bd(unix, 0);
        a.birthday = bd;
        a.mass = jsonBuffer["mass"];
        a.caloricProfile = fromString(jsonBuffer["caloricProfile"]);
        a.zone1Start = jsonBuffer["zone1Start"];
        a.zone2Start = jsonBuffer["zone2Start"];
        a.zone3Start = jsonBuffer["zone3Start"];
        a.zone4Start = jsonBuffer["zone4Start"];
        a.zone5Start = jsonBuffer["zone5Start"];

        model.bio().update(a);
        dataFile.close();
    }
}

void App::saveBikeStats() {
    JsonDocument doc;

    auto& a = model.bike().get();
    
    doc["mass"] = a.mass;
    doc["wheelCircumference"] = a.wheelCircumference;
    doc["logger"] = loggerToString(a.logger);

    if (_storage->exists("/bikeStats.txt"))
        _storage->remove("/bikeStats.txt");

    File32 dataFile = _storage->openFile("/bikeStats.txt", FILE_WRITE);
    if (!dataFile) {
        Serial.println("[App] failed to open /bikeStats.txt for writing");
        return;
    }

    serializeJson(doc, dataFile);
    dataFile.close();
}

void App::loadBikeStats() {
    
    if (_storage->exists("/bikeStats.txt")) {
        Serial.println("Found bikeStats.txt");
        // Open file for reading
        File32 dataFile = _storage->openFile("/bikeStats.txt", FILE_READ);
        if (!dataFile) {
            Serial.println("[App] failed to open /bikeStats.txt for reading");
            return;
        }
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("[App] deserializeJson() failed for /bikeStats.txt: ");
            Serial.println(error.c_str());
            dataFile.close();
            return;
        }

        BikeData a;
        a.mass = jsonBuffer["mass"];
        a.wheelCircumference = jsonBuffer["wheelCircumference"];
        a.logger = loggerFromString(jsonBuffer["logger"]);

        model.bike().update(a);
        dataFile.close();
    }
}

void App::saveLayout() {
    JsonDocument doc;

    auto& l = model.layout().get();

    JsonArray d = doc["displays"].to<JsonArray>();
    doc["rows"] = l.rows;
    doc["cols"] = l.cols;

    for (size_t j = 0; j < l.displays.size(); j++) {
        d[j]["x0"] = l.displays[j].x0;
        d[j]["y0"] = l.displays[j].y0;
        d[j]["x1"] = l.displays[j].x1;
        d[j]["y1"] = l.displays[j].y1;
        d[j]["type"] = toString(l.displays[j].type);
    }

    if (_storage->exists("/layout.txt"))
        _storage->remove("/layout.txt");

    File32 dataFile = _storage->openFile("/layout.txt", FILE_WRITE);
    if (!dataFile) {
        Serial.println("[App] failed to open /layout.txt for writing");
        return;
    }

    serializeJson(doc, dataFile);
    dataFile.close();
}

void App::loadLayout() {
    if (_storage->exists("/layout.txt")) {
        Serial.println("Found layout.txt");
        // Open file for reading
        File32 dataFile = _storage->openFile("/layout.txt", FILE_READ);
        if (!dataFile) {
            Serial.println("[App] failed to open /layout.txt for reading");
            return;
        }
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("[App] deserializeJson() failed for /layout.txt: ");
            Serial.println(error.c_str());
            dataFile.close();
            return;
        }

        std::vector<DisplayItem> displays;
        JsonArray d = jsonBuffer["displays"];
        for (const auto& member : d) {
            DisplayItem disp;
            disp.type = TelemetryTypefromString(member["type"]);
            disp.x0 = member["x0"];
            disp.y0 = member["y0"];
            disp.x1 = member["x1"];
            disp.y1 = member["y1"];
            displays.push_back(disp);
        }

        // Clamp on load rather than making every consumer defend itself.
        // ArduinoJson yields 0 for a missing or unparseable key, and
        // MainScreen::onEnter divides by both of these to compute its grid
        // pitch -- so a truncated layout.txt was a divide by zero on the
        // main screen. DisplayEditScreen already clamped to 2..5; this makes
        // the guarantee hold at the point the data enters the model.
        uint8_t rows = constrain((int)(jsonBuffer["rows"] | 0), LAYOUT_MIN_ROWS, LAYOUT_MAX_ROWS);
        uint8_t cols = constrain((int)(jsonBuffer["cols"] | 0), LAYOUT_MIN_COLS, LAYOUT_MAX_COLS);

        model.layout().update({displays, rows, cols});
        dataFile.close();
    }
}

void App::saveTime() {
    JsonDocument doc;

    int utcOffset = model.time().get().offset();
    doc["UTCOffset"] = utcOffset;

    if (_storage->exists("/time.txt"))
        _storage->remove("/time.txt");

    File32 dataFile = _storage->openFile("/time.txt", FILE_WRITE);
    if (!dataFile) {
        Serial.println("[App] failed to open /time.txt for writing");
        return;
    }

    serializeJson(doc, dataFile);
    dataFile.close();

}

void App::loadTime() {
    if (_storage->exists("/time.txt")) {
        Serial.println("Found time.txt");
        // Open file for reading
        File32 dataFile = _storage->openFile("/time.txt", FILE_READ);
        if (!dataFile) {
            Serial.println("[App] failed to open /time.txt for reading");
            return;
        }
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("[App] deserializeJson() failed for /time.txt: ");
            Serial.println(error.c_str());
            dataFile.close();
            return;
        }
        int UTCOffset = jsonBuffer["UTCOffset"];
        model.time().setUTCOffset(UTCOffset);
    }
}
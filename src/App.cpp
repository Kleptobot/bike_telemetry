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

    //instantiate an instance of both loggers
    //tcxLogger = new TCXLogger(_storage, model);
    fitLogger = new FITLogger(_storage);

    //set the logger interface
    _logger = fitLogger;

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
    Telemetry tel = model.telemetry().get();
    const timeData& currentTime = model.time().get();

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

    if (tel.validLocation && ! validLoc_prev){
        startMessageConfig = true;

    }
    validLoc_prev = tel.validLocation;
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
            ui.showScreen(ScreenID::MainMenu);
            state = AppState::IDLE;
            break;

        case AppState::IDLE:
            if (!HAL::inst().bluetooth().all_devices_discovered())
                HAL::inst().bluetooth().setMode(E_Type_BT_Mode::connect);
            else
                HAL::inst().bluetooth().setMode(E_Type_BT_Mode::idle);
            if(state_prev==AppState::LOGGING) {
                _logger->finaliseLogging();
                model.logger().update({timeDuration(0), timeDuration(0)});
            }
            
            model.telemetry().resetDistance();

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
                model.telemetry().resetDistance();
            }

            model.logger().update({_logger->elapsed_Total(),_logger->elapsed_Lap()});
            
            //check if seconds has changed for logging tick
            if (currentTime.second() != lastSecond) {
                _logger->addTrackpoint({ currentTime,
                                        tel.latitude,
                                        tel.longitude,
                                        tel.altitude,
                                        tel.heartrate,
                                        tel.power,
                                        tel.cadence,
                                        tel.speed,
                                        tel.totalDistance});
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
    float distance = 0;

    // /Haversine formula
    auto gpsLoc = HAL::inst().getGPSLocation();
    auto gpsNow = HAL::inst().getGPSTime();
    auto rtcNow = HAL::inst().getRTCtime();

    if (rtcNow.secondstime() != _lastSeconds) {
        if ( gpsLoc.isValid() && _lastLocation.isValid()) {
            double deg2rad = M_PI/180.0;
            double theta1 = _lastLocation.lat()*deg2rad;
            double theta2 = gpsLoc.lat()*deg2rad;
            double phi1 = _lastLocation.lng()*deg2rad;
            double phi2 = gpsLoc.lng()*deg2rad;

            double s1 = sin((theta2 - theta1)/2.0);
            s1 = s1*s1;
            double c1 = cos(theta1) * cos(theta2);
            double s2 = sin((phi2-phi1)/2.0);
            s2 = s2*s2;

            distance = 2.0*6371000.0*asin(sqrt(s1+c1*s2)); //distance in m
        }
        _lastLocation = gpsLoc;
        _lastSeconds = rtcNow.secondstime();
    }

    auto wheelRPM = HAL::inst().getWheelRPM();
    auto gpsSpeed = HAL::inst().getGPSSpeed();
    auto altVelocity = HAL::inst().altVelocity();

    float speed = 0;
    float circumference = model.bike().get().wheelCircumference;

    // Treat a zero circumference as "no wheel data" rather than computing a
    // speed of zero from it. Without this, an unconfigured circumference
    // silently suppressed the GPS fallback: wheelRPM.live is true whenever a
    // CSC sensor is connected, so the first branch was taken and produced 0.
    if (wheelRPM.live && circumference > 0) {
        speed = wheelRPM.value * circumference * 0.00006;
    } else if (gpsSpeed.live) {
        speed = gpsSpeed.value;
    }

    float grade = 0;
    if (speed >0 && altVelocity.live)
    {
        //rise in m/s * 3.6 to convert to km/h, then divide by speed in km/h to get grade as a percentage
        grade = altVelocity.value*3.6f*100.0f/speed;
    }

    float gearRatio = HAL::inst().getCadence()/wheelRPM.value;

    model.telemetry().update({  HAL::inst().getIMUData(),
                                HAL::inst().getDPSData(),
                                HAL::inst().getBatteryPercentage(),
                                speed,
                                HAL::inst().getCadence(),
                                HAL::inst().getTemperature(),
                                HAL::inst().getAltitude(),
                                HAL::inst().getHeartRate(),
                                HAL::inst().getPower(),
                                gpsLoc.isValid(),
                                gpsLoc.lng(),
                                gpsLoc.lat(),
                                distance,
                                grade});

    //when gps time goes valid, check if the RTC time needs to be re-synced
    int UTCoffset = model.time().get().offset();
    if (gpsLoc.isValid() && !_gpsNowValid) {
        //load gpsTime into _gpsNow DateTime object, adding in the saved UTC offset
        _gpsNow = {rtcNow.year(), rtcNow.month(), rtcNow.day(), (uint8_t)((int)gpsNow.hour()+UTCoffset*60), gpsNow.minute(), gpsNow.second()};
        TimeSpan ts = _gpsNow - rtcNow;
        if (ts.totalseconds() > 30 || ts.totalseconds() < -30)
            HAL::inst().setTime(_gpsNow);
    }
    _gpsNowValid = gpsLoc.isValid();
    
    model.time().update({rtcNow, UTCoffset});
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

    serializeJson(doc, dataFile);
    dataFile.close();
}

void App::loadBiometrics() {
    
    if (_storage->exists("biometrics.txt")) {
        Serial.println("Found biometrics.txt");
        // Open file for reading
        File32 dataFile = _storage->openFile("/biometrics.txt", FILE_READ);
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("deserializeJson() failed: ");
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

    if (_storage->exists("/bikeStats.txt"))
        _storage->remove("/bikeStats.txt");

    File32 dataFile = _storage->openFile("/bikeStats.txt", FILE_WRITE);

    serializeJson(doc, dataFile);
    dataFile.close();
}

void App::loadBikeStats() {
    
    if (_storage->exists("bikeStats.txt")) {
        Serial.println("Found bikeStats.txt");
        // Open file for reading
        File32 dataFile = _storage->openFile("/bikeStats.txt", FILE_READ);
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            return;
        }

        BikeData a;
        a.mass = jsonBuffer["mass"];
        a.wheelCircumference = jsonBuffer["wheelCircumference"];

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

    for (int j = 0; j < l.displays.size(); j++) {
        d[j]["x0"] = l.displays[j].x0;
        d[j]["y0"] = l.displays[j].y0;
        d[j]["x1"] = l.displays[j].x1;
        d[j]["y1"] = l.displays[j].y1;
        d[j]["type"] = toString(l.displays[j].type);
    }

    if (_storage->exists("/layout.txt"))
        _storage->remove("/layout.txt");

    File32 dataFile = _storage->openFile("/layout.txt", FILE_WRITE);

    serializeJson(doc, dataFile);
    dataFile.close();
}

void App::loadLayout() {
    if (_storage->exists("layout.txt")) {
        Serial.println("Found layout.txt");
        // Open file for reading
        File32 dataFile = _storage->openFile("/layout.txt", FILE_READ);
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("deserializeJson() failed: ");
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

        uint8_t rows = jsonBuffer["rows"];
        uint8_t cols = jsonBuffer["cols"];

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

    serializeJson(doc, dataFile);
    dataFile.close();

}

void App::loadTime() {
    if (_storage->exists("time.txt")) {
        Serial.println("Found time.txt");
        // Open file for reading
        File32 dataFile = _storage->openFile("/time.txt", FILE_READ);
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            return;
        }
        int UTCOffset = jsonBuffer["UTCOffset"];
        model.time().setUTCOffset(UTCOffset);
    }
}
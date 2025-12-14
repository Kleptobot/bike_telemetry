#include <Arduino.h>
#include "App.hpp"
#include <variant>
#include <cmath>

#include "UI/Screens/MainScreen.hpp"
#include "UI/Screens/BluetoothScreen.hpp"
#include "UI/Screens/TimeEditScreen.hpp"
#include "UI/Screens/SettingsScreen.hpp"
#include "UI/Screens/BiometricsScreen.hpp"
#include "UI/Screens/DisplayEditScreen.hpp"
#include "UI/Screens/GPSScreen.hpp"

void App::begin(IStorage* storage) {
    _storage = storage;
    logger = new TCXLogger(_storage, model);
    state = AppState::BOOT;
    Disp::init();
    loadLayout();

    ui.registerScreen<MainScreen>(ScreenID::MainMenu,App::instance().getModel());
    ui.registerScreen<TimeEditScreen>(ScreenID::TimeMenu,App::instance().getModel());
    ui.registerScreen<SettingsScreen>(ScreenID::SettingsMenu,App::instance().getModel());
    ui.registerScreen<BluetoothScreen>(ScreenID::Bluetooth,App::instance().getModel());
    ui.registerScreen<BiometricsScreen>(ScreenID::Biometrics,App::instance().getModel());
    ui.registerScreen<DisplayEditScreen>(ScreenID::DisplayEdit,App::instance().getModel());
    ui.registerScreen<GPSScreen>(ScreenID::GPSSettings,App::instance().getModel());

    ui.begin(ScreenID::MainMenu);

    // Register callback with HAL
    HAL::onTelemetry([this](imu_data imu, dps_data dps, float speed, float cadence, float temp, float alt, float bpm, float pow, TinyGPSLocation loc, DateTime now) {
        this->updateTelemetry(imu, dps, speed, cadence, temp, alt, bpm, pow, loc, now);
    });

    HAL::bluetooth().onDeviceList([this](std::vector<BluetoothDevice> devices) {
        this->updateBluetooth(devices);
    });

    InputSystem::onEnableStateRead([this](bool pinState) {
        this->updateGpsEnable(pinState);
    });

    Serial.println("[App] Started and registered telemetry callback.");
}

void App::update() {
    // Any periodic application-level behavior here
    Telemetry tel = model.telemetry().get();
    DateTime currentTime = model.time().get();

    if (!appEvents.empty()) {
        handleAppEvent(appEvents.front());
        appEvents.pop();
    }
    _millis = millis();
    ui.render();
    ui.update((float)(_millis - _last_millis) / 1000.0);
    ui.handleInput(HAL::inputs());
    _last_millis = _millis;

    switch(state) {
        case AppState::BOOT:
            Serial.println("Loading data from filesystem.");
            HAL::bluetooth().loadDevices();
            loadBiometrics();
            state = AppState::IDLE;
            break;

        case AppState::IDLE:
            if (!HAL::bluetooth().all_devices_discovered())
                HAL::bluetooth().setMode(E_Type_BT_Mode::connect);
            else
                HAL::bluetooth().setMode(E_Type_BT_Mode::idle);
            if(state_prev==AppState::LOGGING) {
                logger->finaliseLogging();
                model.logger().update({0,0});
            }
            
            f32_distance = 0;
            break;

        case AppState::CONFIG:
            HAL::bluetooth().setMode(E_Type_BT_Mode::scan);
            break;

        case AppState::LOGGING:
            HAL::bluetooth().setMode(E_Type_BT_Mode::idle);

            if(state != state_prev)
                logger->startLogging(currentTime);

            model.logger().update({logger->elapsed_Total(),logger->elapsed_Lap()});
            
            //check if seconds has changed for logging tick
            if (currentTime.second() != lastSecond) {
                f32_distance += tel.distance;
                logger->addTrackpoint({ currentTime,
                                        tel.latitude,
                                        tel.longitude,
                                        tel.altitude,
                                        tel.heartrate,
                                        tel.power,
                                        tel.cadence,
                                        tel.speed,
                                        f32_distance});
            }
            break;
        
        case AppState::PAUSED:
            break;
        
        default:
            break;

    }
    lastSecond = currentTime.second();
    state_prev = state;
    model.app().setState(state);
}

void App::updateTelemetry(imu_data imu, dps_data dps, float speed, float cadence, float temp, float alt, float bpm, float pow, TinyGPSLocation loc, DateTime now) {
    float distance = 0;

    // /Haversine formula
    if ( loc.isValid() && _lastLocation.isValid()) {
        double deg2rad = M_PI/180.0;
        double theta1 = _lastLocation.lat()*deg2rad;
        double theta2 = loc.lat()*deg2rad;
        double phi1 = _lastLocation.lng()*deg2rad;
        double phi2 = loc.lng()*deg2rad;

        double s1 = sin((theta2 - theta1)/2.0);
        s1 = s1*s1;
        double c1 = cos(theta1) * cos(theta2);
        double s2 = sin((phi2-phi1)/2.0);
        s2 = s2*s2;

        distance = 2.0*6371.0*asin(sqrt(s1+c1*s2));
    }
    _lastLocation = loc;
    model.telemetry().update({imu,dps,speed,cadence,temp,alt,bpm,pow,loc.isValid(),loc.lng(),loc.lat(), distance});
    model.time().update(now);
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
            HAL::setTime(std::get<DateTime>(e.payload));
            break;

        case AppEventType::SaveBiometrics:
            saveBiometrics();
            break;

        case AppEventType::SaveLayout:
            saveLayout();
            break;

        case AppEventType::StartLogging:
            HAL::bluetooth().setMode(E_Type_BT_Mode::idle);
            state = AppState::LOGGING;
            break;

        case AppEventType::StopLogging:
            state = AppState::IDLE;
            break;

        case AppEventType::ConnectBluetooth:
            HAL::bluetooth().createDevice(std::get<BluetoothDevice>(e.payload));
            break;

        case AppEventType::DisconnectBluetooth:
            HAL::bluetooth().disconnectDevice(std::get<BluetoothDevice>(e.payload));
            break;

        case AppEventType::ScanBluetooth:
            state = AppState::IDLE;
            break;

        case AppEventType::DiscoverBluetooth:
            state = AppState::CONFIG;
            break;

        case AppEventType::Sleep:
            HAL::sleep();
        default:
            break;
    }
}

void App::saveBiometrics() {
    JsonDocument doc;

    auto& a = model.app().get();
    
    doc["birthday"] = a.birthday.unixtime();
    doc["mass"] = a.mass;
    doc["caloricProfile"] = toString(a.caloricProfile);

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

        AppData a;
        uint32_t unix = jsonBuffer["birthday"];
        DateTime bd(unix);
        a.birthday = bd;
        a.mass = jsonBuffer["mass"];
        a.caloricProfile = fromString(jsonBuffer["caloricProfile"]);

        model.app().update(a);
        dataFile.close();
    }
}

void App::saveLayout() {
    JsonDocument doc;

    auto& l = model.layout().get();

    JsonArray d = doc["displays"].to<JsonArray>();

    for (auto member : l.displays) {
        d.add(toString(member));
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

        std::vector<TelemetryType> displays;
        JsonArray d = jsonBuffer["displays"];
        for (const auto& member : d) {
            displays.push_back(TelemetryTypefromString(member));
        }

        model.layout().update({displays});
        dataFile.close();
    }
}
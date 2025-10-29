#include <Arduino.h>
#include "App.hpp"
#include <variant>

void App::begin(UIManager* uiManager, IStorage* storage) {
    ui = uiManager;
    _storage = storage;
    logger = new TCXLogger(_storage);
    state = AppState::IDLE;

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
    switch(state) {
        case AppState::BOOT:
            HAL::bluetooth().loadDevices();
            loadBiometrics();
            logger->setAge(TimeSpan(currentTime - model.app().get().birthday).days()/356.25);
            state = AppState::IDLE;
            break;

        case AppState::IDLE:
            if (!HAL::bluetooth().all_devices_discovered())
                HAL::bluetooth().setMode(E_Type_BT_Mode::connect);
            else
                HAL::bluetooth().setMode(E_Type_BT_Mode::idle);
            if(state_prev==AppState::LOGGING)
                logger->finaliseLogging();
            break;

        case AppState::CONFIG:
            HAL::bluetooth().setMode(E_Type_BT_Mode::scan);
            break;

        case AppState::LOGGING:
            if(state != state_prev)
                logger->startLogging(currentTime);
            
            f32_distance += (tel.speed + lastSpeed)*(0.5/3.6);
            lastSpeed = tel.speed;
            //check if seconds has changed for logging tick
            if (currentTime.second() != lastSecond)
                logger->addTrackpoint({ currentTime,
                                        tel.longitude,
                                        tel.longitude,
                                        tel.altitude,
                                        tel.heartrate,
                                        tel.power,
                                        tel.cadence,
                                        tel.speed,
                                        f32_distance});
            break;
        
        case AppState::PAUSED:
            break;
        
        default:
            break;

    }
    lastSecond = currentTime.second();
    state_prev = state;
}

void App::updateTelemetry(imu_data imu, dps_data dps, float speed, float cadence, float temp, float alt, float bpm, float pow, TinyGPSLocation loc, DateTime now) {
    model.telemetry().update({imu,dps,speed,cadence,temp,alt,bpm,pow,loc.isValid(),loc.lng(),loc.lat()});
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
            HAL::setTime(model.time().get());
            break;

        case AppEventType::SaveBiometrics:
            saveBiometrics();
            break;

        case AppEventType::StartLogging:
            HAL::bluetooth().setMode(E_Type_BT_Mode::idle);
            state = AppState::LOGGING;
            break;

        case AppEventType::StopLogging:
            
            break;

        case AppEventType::ConnectBluetooth:
            HAL::bluetooth().createDevice(std::get<BluetoothDevice>(e.payload));
            break;

        case AppEventType::DisconnectBluetooth:
            HAL::bluetooth().disconnectDevice(std::get<BluetoothDevice>(e.payload));

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
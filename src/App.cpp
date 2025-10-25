#include "App.hpp"
// #include "UI/Screens/MainScreen.h"
// #include "UI/Screens/LoggingScreen.h"
#include "display/Display.hpp"
#include <Arduino.h>

void App::begin(UIManager* uiManager, IStorage* storage) {
    ui = uiManager;
    _storage = storage;
    logger = new TCXLogger(_storage);
    state = AppState::IDLE;

    // Register callback with HAL
    HAL::onTelemetry([this](imu_data imu, float speed, float cadence, float temp, float alt, float bpm, TinyGPSLocation loc, DateTime now) {
        this->updateTelemetry(imu, speed, cadence, temp, alt, bpm, loc, now);
    });

    HAL::onBluetooth([this](std::vector<BluetoothDevice> devices) {
        this->updateBluetooth(devices);
    });

    InputSystem::onEnableStateRead([this](bool pinState) {
        this->updateGpsEnable(pinState);
    });

    Serial.println("[App] Started and registered telemetry callback.");
}

void App::update() {
    // Any periodic application-level behavior here
    if (state == AppState::LOGGING) {
        f32_distance += (currentSpeed + lastSpeed)*(0.5/3.6);
        lastSpeed = currentSpeed;
        //check if seconds has changed for logging tick
        if (currentTime.second() != lastSecond)
            logger->addTrackpoint({currentTime,currentLocation.lat(),currentLocation.lng(),currentAlt,currentBPM,0,currentCadence,currentSpeed,f32_distance});
        lastSecond = currentTime.second();
    }
    _inputs = HAL::inputs();
}

void App::updateTelemetry(imu_data imu, float speed, float cadence, float temp, float alt, float bpm, TinyGPSLocation loc, DateTime now) {
    currentSpeed = speed;
    currentCadence = cadence;
    currentBPM = bpm;
    currentTemp = temp;
    currentAlt = alt;
    currentLocation = loc;
    currentIMU = imu;
    currentTime = now;
}

void App::updateBluetooth(std::vector<BluetoothDevice> devices) {
    _devices = devices;
}

void App::updateGpsEnable(bool state) {
    _gpsEnableState = state;
}

void App::transitionTo(AppState newState) {
    state = newState;

    // switch (state) {
    //     case AppState::IDLE:
    //         ui->setScreen(UIManager::ScreenID::MAIN);
    //         break;
    //     case AppState::LOGGING:
    //         ui->setScreen(UIManager::ScreenID::LOGGING);
    //         break;
    //     case AppState::PAUSED:
    //         ui->setScreen(UIManager::ScreenID::PAUSED);
    //         break;
    //     default:
    //         break;
    // }
}

void App::handleAppEvent(const AppEvent& e) {
    switch (e.type) {
        case AppEventType::SaveTime:
            DateTime* pdate = static_cast<DateTime*>(e.data);
            HAL::setTime(*pdate);
            break;

        case AppEventType::StartLogging:
            logger->startLogging(currentTime);
            break;

        case AppEventType::StopLogging:
            logger->finaliseLogging();
            break;

        case AppEventType::ConnectBluetooth:
            //hal.Bluetooth().connect();
            break;

        default:
            break;
    }
}
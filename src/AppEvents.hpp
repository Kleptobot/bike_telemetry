#pragma once

#include <Arduino.h>
#include <variant>
#include <RTClib.h>
#include "HAL/BluetoothInterface.hpp"

enum class AppEventType {
    None,
    SaveTime,
    SaveBiometrics,
    SaveLayout,
    StartLogging,
    StopLogging,
    PauseLogging,
    FactoryReset,
    ConnectBluetooth,
    DisconnectBluetooth,
    DiscoverBluetooth,
    ScanBluetooth,
    RestoreDefaultsGPS,
    ResetGPS,
    Sleep
};

struct AppEvent {
    AppEventType type;
    std::variant<int,float,BluetoothDevice,DateTime> payload;
};
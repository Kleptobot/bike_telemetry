#pragma once

#include <Arduino.h>
#include <variant>
#include "HAL/BluetoothInterface.hpp"

enum class AppEventType {
    None,
    SaveTime,
    StartLogging,
    StopLogging,
    PauseLogging,
    FactoryReset,
    ConnectBluetooth,
    DisconnectBluetooth,
    Sleep
};

struct AppEvent {
    AppEventType type;
    std::variant<int,float,BluetoothDevice> payload;
};
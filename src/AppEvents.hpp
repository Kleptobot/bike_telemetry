#pragma once
#include <Arduino.h>
#include <mpark/variant.hpp>
#include "HAL/BluetoothInterface.hpp"

enum class AppEventType {
    None,
    SaveTime,
    StartLogging,
    StopLogging,
    FactoryReset,
    ConnectBluetooth,
    DisconnectBluetooth
};

struct AppEvent {
    AppEventType type;
    mpark::variant<int,float,BluetoothDevice> payload;
};
#pragma once

#include <Arduino.h>
#include <variant>
#include <RTClib.h>
#include "HAL/BluetoothInterface.hpp"
#include "DataModel/TimeDataProvider.hpp"

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
    saveGPSNVRAM,
    setGPSNMEARate,
    UnmountSD,
    Sleep
};

struct NMEArateChange {
    int type;
    int rate;
};

struct AppEvent {
    AppEventType type;
    std::variant<int,float,BluetoothDevice,timeData,NMEArateChange> payload;
};
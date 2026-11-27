#ifndef APPEVENTS_H
#define APPEVENTS_H

#include <Arduino.h>
#include <variant>
#include "HAL/BluetoothInterface.hpp"
#include "DataModel/TimeDataProvider.hpp"
#include "DataModel/GpsSettingsProvider.hpp"

enum class AppEventType {
    None,
    SaveTime,
    SaveBiometrics,
    SaveLayout,
    StartLogging,
    StopLogging,
    PauseLogging,
    ResumeLogging,
    NewLap,
    FactoryReset,
    ConnectBluetooth,
    DisconnectBluetooth,
    DiscoverBluetooth,
    ScanBluetooth,
    RestoreDefaultsGPS,
    ResetGPS,
    SaveGPSConfig,
    UnmountSD,
    Sleep,
    SaveBikeStats
};

struct AppEvent {
    AppEventType type;
    std::variant<int,float,BluetoothDevice,timeData,GpsSettings> payload;
};

#endif /* APPEVENTS_H */
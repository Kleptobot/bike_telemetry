#pragma once

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
    void* data;
};
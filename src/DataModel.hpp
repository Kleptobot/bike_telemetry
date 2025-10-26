#pragma once
#include "TelemetryDataProvider.hpp"
#include "BluetoothDataProvider.hpp"
#include "TimeDataProvider.hpp"
#include "AppDataProvider.hpp"
// #include "SettingsDataProvider.h"

class DataModel {
public:
    TelemetryDataProvider& telemetry() { return _telemetry; }
    BluetoothDataProvider& bluetooth() { return _bluetooth; }
    TimeDataProvider& time() { return _time; }
    AppDataProvider& app() { return _app; }
    // SettingsDataProvider& settings() { return _settings; }

    const TelemetryDataProvider& telemetry() const { return _telemetry; }
    const BluetoothDataProvider& bluetooth() const { return _bluetooth; }
    const TimeDataProvider& time() const { return _time; }
    const AppDataProvider& app() const { return _app; }
    // const SettingsDataProvider& settings() const { return _settings; }

private:
    TelemetryDataProvider _telemetry;
    BluetoothDataProvider _bluetooth;
    TimeDataProvider _time;
    AppDataProvider _app;
    // SettingsDataProvider _settings;
};

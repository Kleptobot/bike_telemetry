#pragma once
#include "TelemetryDataProvider.hpp"
#include "BluetoothDataProvider.hpp"
#include "TimeDataProvider.hpp"
#include "AppDataProvider.hpp"
#include "LayoutDataProvider.hpp"
#include "LoggerDataProvider.hpp"

class DataModel {
public:
    TelemetryDataProvider& telemetry() { return _telemetry; }
    BluetoothDataProvider& bluetooth() { return _bluetooth; }
    TimeDataProvider& time() { return _time; }
    AppDataProvider& app() { return _app; }
    LayoutDataProvider& layout() { return _layout; }
    LoggerDataProvider& logger() { return _logger; }

    const TelemetryDataProvider& telemetry() const { return _telemetry; }
    const BluetoothDataProvider& bluetooth() const { return _bluetooth; }
    const TimeDataProvider& time() const { return _time; }
    const AppDataProvider& app() const { return _app; }
    const LayoutDataProvider& layout() const { return _layout; }
    const LoggerDataProvider& logger() const { return _logger; }

private:
    TelemetryDataProvider _telemetry;
    BluetoothDataProvider _bluetooth;
    TimeDataProvider _time;
    AppDataProvider _app;
    LayoutDataProvider _layout;
    LoggerDataProvider _logger;
};

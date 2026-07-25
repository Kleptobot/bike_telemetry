#ifndef DATAMDOEL_H
#define DATAMDOEL_H

#include "TelemetryDataProvider.hpp"
#include "BluetoothDataProvider.hpp"
#include "TimeDataProvider.hpp"
#include "BioDataProvider.hpp"
#include "LayoutDataProvider.hpp"
#include "LoggerDataProvider.hpp"
#include "SDStateProvider.hpp"
#include "BikeDataProvider.hpp"

enum class AppState {
    BOOT,
    IDLE,
    CONFIG,
    LOGGING,
    PAUSED
};

class DataModel {
public:
    TelemetryDataProvider& telemetry() { return _telemetry; }
    BluetoothDataProvider& bluetooth() { return _bluetooth; }
    TimeDataProvider& time() { return _time; }
    BioDataProvider& bio() { return _bio; }
    LayoutDataProvider& layout() { return _layout; }
    LoggerDataProvider& logger() { return _logger; }
    SDStateProvider& SD() { return _SD; }
    BikeDataProvider& bike() { return _bike; }
    AppState& app() { return _appState; }
    void setAppState( AppState new_) { _appState = new_; }

    const TelemetryDataProvider& telemetry() const { return _telemetry; }
    const BluetoothDataProvider& bluetooth() const { return _bluetooth; }
    const TimeDataProvider& time() const { return _time; }
    const BioDataProvider& bio() const { return _bio; }
    const LayoutDataProvider& layout() const { return _layout; }
    const LoggerDataProvider& logger() const { return _logger; }
    const SDStateProvider& SD() const { return _SD; }
    const BikeDataProvider& bike() const { return _bike; }
    const AppState& app() const { return _appState; }

private:
    TelemetryDataProvider _telemetry;
    BluetoothDataProvider _bluetooth;
    TimeDataProvider _time;
    BioDataProvider _bio;
    BikeDataProvider _bike;
    LayoutDataProvider _layout;
    LoggerDataProvider _logger;
    SDStateProvider _SD;
    AppState _appState;
};

#endif
#ifndef DATAMDOEL_H
#define DATAMDOEL_H

#include "BluetoothDataProvider.hpp"
#include "TimeDataProvider.hpp"
#include "BioDataProvider.hpp"
#include "LayoutDataProvider.hpp"
#include "LoggerDataProvider.hpp"
#include "SDStateProvider.hpp"
#include "BikeDataProvider.hpp"
#include "DataBus.hpp"
#include "GpsTrack.hpp"

enum class AppState {
    BOOT,
    IDLE,
    CONFIG,
    LOGGING,
    PAUSED
};

class DataModel {
public:
    BluetoothDataProvider& bluetooth() { return _bluetooth; }
    TimeDataProvider& time() { return _time; }
    BioDataProvider& bio() { return _bio; }
    LayoutDataProvider& layout() { return _layout; }
    LoggerDataProvider& logger() { return _logger; }
    SDStateProvider& SD() { return _SD; }
    BikeDataProvider& bike() { return _bike; }
    AppState& app() { return _appState; }
    void setAppState( AppState new_) { _appState = new_; }

    // DataBus for pub/sub access to derived signals.
    DataBus& bus() { return _bus; }
    const DataBus& bus() const { return _bus; }

    // Recent-position breadcrumb store for the map widget. App appends on
    // each valid GPS fix; the store decimates to 1 Hz internally.
    GpsTrack& gpsTrack() { return _gpsTrack; }
    const GpsTrack& gpsTrack() const { return _gpsTrack; }

    const BluetoothDataProvider& bluetooth() const { return _bluetooth; }
    const TimeDataProvider& time() const { return _time; }
    const BioDataProvider& bio() const { return _bio; }
    const LayoutDataProvider& layout() const { return _layout; }
    const LoggerDataProvider& logger() const { return _logger; }
    const SDStateProvider& SD() const { return _SD; }
    const BikeDataProvider& bike() const { return _bike; }
    const AppState& app() const { return _appState; }

private:
    BluetoothDataProvider _bluetooth;
    TimeDataProvider _time;
    BioDataProvider _bio;
    BikeDataProvider _bike;
    LayoutDataProvider _layout;
    LoggerDataProvider _logger;
    SDStateProvider _SD;
    AppState _appState;
    DataBus _bus;
    GpsTrack _gpsTrack;
};

#endif
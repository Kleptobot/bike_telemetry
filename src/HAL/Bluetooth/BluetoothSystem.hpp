#pragma once
#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include <functional>

#include "BT_Device.hpp"
#include "csc.hpp"
#include "hrm.hpp"
#include "cps.hpp"
#include "HAL/BluetoothInterface.hpp"
#include "HAL/StorageInterface.hpp"

class BluetoothSystem {
  public:
    static void init(IStorage* storage);
    static void update();
    static const std::vector<BluetoothDevice>& devices() { return deviceList; }
    static void createDevice(const BluetoothDevice& device);
    static void disconnectDevice(const BluetoothDevice& device);
    static void setMode(E_Type_BT_Mode mode) { _mode = mode; }

    static void loadDevices();
    static void saveDevices();

    static bool all_devices_discovered() { return BT_Device::all_devices_discovered(); };

    using DeviceListCallback = std::function<void(std::vector<BluetoothDevice> devices)>;
    static void onDeviceList(DeviceListCallback cb) { deviceListCallback = cb; }

  private:
    static std::vector<BluetoothDevice> deviceList;
    static E_Type_BT_Mode _mode, _mode_prev;

    static void connect_callback(uint16_t conn_handle);
    static void scan_callback(ble_gap_evt_adv_report_t* report);
    static void scan_discovery(ble_gap_evt_adv_report_t* report);
    
    static MacAddress toConnectMAC;
    
    static DeviceListCallback deviceListCallback;
    static IStorage* _storage;
};
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

/**
 * @class BluetoothSystem
 * @brief manages creation and destruction of BT_Device objects
 * 
 * Provides methods for a high level system to interact with BT_Devices
 * provides functions for saving and loading, and manages the concept of devices that have been discovered but not connected
 */
class BluetoothSystem {
  public:
    using DeviceListCallback = std::function<void(std::vector<BluetoothDevice> devices)>;

    /**
     * initialises the bluetooth system
     * sets callbacks, the advertising name, and how many devices can be cincurrently connected
     * @param storage pointer to a storage interface for file system operations
     */
    static void init(IStorage* storage);

    /**
     * Runs the main state machine of the bluetooth system
     * performs actions like checking the connected state of devices and starting/stopping scanning
     */
    static void update();

    /**
     * Returns a vector of BluetoothDevices
     */
    static const std::vector<BluetoothDevice>& devices() { return deviceList; }

    /**
     * Checks the internal list of BT_Devices
     * if a device with the MAC address in the new device is not found
     * a new BT_Device of the corresponding type is created
     * @param device Bluetooth device to be created
     */
    static void createDevice(const BluetoothDevice& device);

    /**
     * Checks the device list for a maching device
     * if found removes the device from the list
     * @param device Bluetooth device to be removed
     */
    static void disconnectDevice(const BluetoothDevice& device);

    /**
     * Sets the bluetooth system mode
     * @param mode Enum state to set as the new state
     */
    static void setMode(E_Type_BT_Mode mode) { _mode = mode; }

    /**
     * Load devices from file
     * devices are stored in plain text as json
     * the format is [{"name":<device name>, "type": <device type>, "MAC": [<oct1>,<oct2>,<oct3>,<oct4>,<oct5>,<oct6>]}, ...]
     */
    static void loadDevices();

    /**
     * Saves devices to file
     * devices are stored in plain text as json
     * the format is [{"name":<device name>, "type": <device type>, "MAC": [<oct1>,<oct2>,<oct3>,<oct4>,<oct5>,<oct6>]}, ...]
     */
    static void saveDevices();

    /**
     * returns true if all BT_Devices have been discovered
     */
    static bool all_devices_discovered() { return BT_Device::all_devices_discovered(); };

    /**
     * callback invoked when the devices vector is updated
     */
    static void onDeviceList(DeviceListCallback cb) { deviceListCallback = cb; }

  private:
    static std::vector<BluetoothDevice> deviceList;
    static E_Type_BT_Mode _mode, _mode_prev;
    static MacAddress toConnectMAC;
    static DeviceListCallback deviceListCallback;
    static IStorage* _storage;


    /**
     * Callback invoked when an connection is established
     * @param conn_handle
     */
    static void connect_callback(uint16_t conn_handle);

    /**
     * Callback invoked when scanner pick up an advertising data
     * @param report Structural advertising data
     */
    static void scan_callback(ble_gap_evt_adv_report_t* report);

    /**
     * Callback invoked when scanner pick up an advertising data
     * @param report Structural advertising data
     */
    static void scan_discovery(ble_gap_evt_adv_report_t* report);
};
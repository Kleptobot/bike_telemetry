#pragma once
#include <Arduino.h>
#include <vector>
#include <SdFat.h>
#include <ArduinoJson.h>

#include "BT_Device.hpp"
#include "csc.hpp"
#include "hrm.hpp"
#include "cps.hpp"
#include "HAL/BluetoothInterface.hpp"

typedef struct
{
  char name[32];
  uint8_t MAC[6];
  uint16_t batt;
  E_Type_BT_Device type;
  const BT_Device* device = nullptr;
  bool connected;
} BluetoothDevice;

class BluetoothManager {
public:
  static void init();
  static void update();
  static const std::vector<BluetoothDevice>& devices() { return deviceList; }

private:
    static std::vector<BluetoothDevice> deviceList;
    static void connectDevice(BluetoothDevice* device);
    static void disconnectDevice(BluetoothDevice* device);

    static void connect_callback(uint16_t conn_handle);
    static void scan_callback(ble_gap_evt_adv_report_t* report);
    static void scan_discovery(ble_gap_evt_adv_report_t* report);
    
    static uint8_t toConnectMAC[6];

    static void loadDevices(SdFat32* SD);
    static void saveDevices(SdFat32* SD);
};
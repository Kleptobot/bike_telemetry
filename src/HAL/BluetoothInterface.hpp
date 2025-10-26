#pragma once
#include <Arduino.h>
#include "MAC.hpp"

enum E_Type_BT_Device {
  bt_csc,
  bt_hrm,
  bt_cps
};

enum E_Type_BT_Mode {
  idle,
  scan,
  connect
};

struct BluetoothDevice {
  char name[32];
  MacAddress MAC;
  uint16_t batt;
  E_Type_BT_Device type;
  bool connected;
  bool saved;

  BluetoothDevice(){}
  BluetoothDevice(uint8_t* macAddr) : MAC(macAddr) {}
} ;
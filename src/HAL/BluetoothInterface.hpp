#pragma once
#include <Arduino.h>

enum E_Type_BT_Device{
  bt_csc,
  bt_hrm,
  bt_cps
};

typedef struct
{
  char name[32];
  uint8_t MAC[6];
  uint16_t batt;
  E_Type_BT_Device type;
  bool connected;
} BluetoothDevice;
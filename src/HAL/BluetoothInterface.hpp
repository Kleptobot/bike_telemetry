#ifndef BLUETOOTHINTERFACE_H
#define BLUETOOTHINTERFACE_H

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
    char name[32] = {0};
    MacAddress MAC;
    uint16_t batt = 100;
    // Defaulted so a device whose advertisement matches none of the three
    // service UUIDs does not carry an indeterminate type into createDevice().
    E_Type_BT_Device type = E_Type_BT_Device::bt_csc;
    bool connected = false;
    bool saved = false;

    BluetoothDevice(){}
    BluetoothDevice(uint8_t* macAddr, bool conn = false, bool save=false) :
        MAC(macAddr),
        connected(conn),
        saved(save) {}
} ;

#endif /* BLUETOOTHINTERFACE_H */
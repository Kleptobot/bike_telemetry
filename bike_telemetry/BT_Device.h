#ifndef BT_DEVICE
#define BT_DEVICE

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include <functional>
#include <algorithm> 
#include "Utils.h"

#define     GATT_BAT_UUID                           0x180F
#define     GATT_BAT_MEASUREMENT_UUID               0x2A19

typedef enum {
  csc,
  hrm
} E_Type_BT_Device;

class BT_Device {
  private:

  protected:
    BLEClientService        bat_serv  = BLEClientService(GATT_BAT_UUID);
    BLEClientCharacteristic bat_meas  = BLEClientCharacteristic(GATT_BAT_MEASUREMENT_UUID);
    
    String name;
    uint8_t MAC[6];
    uint16_t u16Batt;
    bool _begun;
    uint16_t _conn_handle;
    E_Type_BT_Device type;
    static std::vector<std::unique_ptr<BT_Device>> btDevices;
    BT_Device(){};

  public:
    
    static BT_Device* getDeviceWithMAC(uint8_t* MAC);

    static void removeDeviceWithMAC(uint8_t* MAC);

    static void removeDevice(std::unique_ptr<BT_Device> device);

    static void disconnect_callback(uint16_t conn_handle, uint8_t reason);
    
    static void bat_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    void bat_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    uint8_t *getMac(){return MAC;}
    String getName(){return (String)name;}
    E_Type_BT_Device getType(){return type;}
    bool begun(){return _begun;}
    uint16_t readBatt(){return u16Batt;}
    void disconnect(uint16_t conn_handle, uint8_t reason);
    virtual void discover(uint16_t conn_handle){};
    virtual bool discovered(){return false;};
    virtual void begin(){};
};

 #endif /* BT_DEVICE */
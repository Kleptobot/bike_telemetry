#ifndef BT_DEVICE
#define BT_DEVICE

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include <functional>
#include <algorithm> 
#include "Utils.h"
#include "logger.h"

#define     GATT_BAT_UUID                           0x180F
#define     GATT_BAT_MEASUREMENT_UUID               0x2A19

enum E_Type_BT_Device{
  bt_csc,
  bt_hrm,
  bt_cps
};

class BT_Device {
  private:

  protected:
    BLEClientService        bat_serv  = BLEClientService(GATT_BAT_UUID);
    BLEClientCharacteristic bat_meas  = BLEClientCharacteristic(GATT_BAT_MEASUREMENT_UUID);
    
    char name[32];
    uint8_t MAC[6];
    uint8_t u8_Batt = 100;
    bool _begun;
    uint16_t _conn_handle;
    E_Type_BT_Device bt_type;
    static std::vector<std::unique_ptr<BT_Device>> btDevices;
    BT_Device(){};
    bool _disconnected;

  public:
    virtual ~BT_Device(){Serial.println("deleteing BT_Device");};
    
    static BT_Device* getDeviceWithMAC(uint8_t* MAC);

    static std::unique_ptr<BT_Device> removeDeviceWithMAC(uint8_t* MAC);

    static std::unique_ptr<BT_Device> removeDevice(std::unique_ptr<BT_Device> device);

    static void disconnect_callback(uint16_t conn_handle, uint8_t reason);
    
    static void bat_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    static bool all_devices_discovered();

    void bat_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    uint8_t *getMac(){return MAC;}
    String getName(){return (String)name;}
    E_Type_BT_Device getType(){return bt_type;}
    bool begun(){return _begun;}
    uint8_t readBatt(){return u8_Batt;}
    void disconnect(uint16_t conn_handle, uint8_t reason);
    uint16_t getConnHandle(){return _conn_handle;};
    virtual void discover(uint16_t conn_handle){};
    virtual bool discovered(){return false;};
    virtual void begin(){};
};

 #endif /* BT_DEVICE */
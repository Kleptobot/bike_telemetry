#ifndef BT_DEVICE
#define BT_DEVICE

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include <functional>
#include <algorithm> 

#include "HAL/BluetoothInterface.hpp"
#include "MAC.hpp"

#define     GATT_BAT_UUID                           0x180F
#define     GATT_BAT_MEASUREMENT_UUID               0x2A19
#define     GATT_SENSOR_LOCATION_UUID               0x2A5D

/* Sensor location enum */
#define     SENSOR_LOCATION_OTHER                   0
#define     SENSOR_LOCATION_TOP_OF_SHOE             1
#define     SENSOR_LOCATION_IN_SHOE                 2
#define     SENSOR_LOCATION_HIP                     3
#define     SENSOR_LOCATION_FRONT_WHEEL             4
#define     SENSOR_LOCATION_LEFT_CRANK              5
#define     SENSOR_LOCATION_RIGHT_CRANK             6
#define     SENSOR_LOCATION_LEFT_PEDAL              7
#define     SENSOR_LOCATION_RIGHT_PEDAL             8
#define     SENSOR_LOCATION_FROT_HUB                9
#define     SENSOR_LOCATION_REAR_DROPOUT            10
#define     SENSOR_LOCATION_CHAINSTAY               11
#define     SENSOR_LOCATION_REAR_WHEEL              12
#define     SENSOR_LOCATION_REAR_HUB                13
#define     SENSOR_LOCATION_CHEST                   14
#define     SENSOR_LOCATION_SPIDER                  15
#define     SENSOR_LOCATION_CHAIN_RING              16

class BT_Device {
  private:

  protected:
    BLEClientService        bat_serv  = BLEClientService(GATT_BAT_UUID);
    BLEClientCharacteristic bat_meas  = BLEClientCharacteristic(GATT_BAT_MEASUREMENT_UUID);
    
    MacAddress MAC;
    uint8_t u8_Batt = 100;
    bool _begun;
    uint16_t _conn_handle;
    E_Type_BT_Device bt_type;
    static std::vector<std::unique_ptr<BT_Device>> btDevices;
    bool _disconnected;

    BT_Device(){};

  public:
    virtual ~BT_Device() {};
    
    static BT_Device* getDeviceWithMAC(MacAddress MAC);

    static void removeDeviceWithMAC(MacAddress MAC);

    static void removeDevice(std::unique_ptr<BT_Device> device);

    static void disconnect_callback(uint16_t conn_handle, uint8_t reason);
    
    static void bat_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    static bool all_devices_discovered();

    static bool deviceWithMacDiscovered(MacAddress MAC);

    void bat_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    virtual void update(uint32_t now) {};

    MacAddress getMac() {return MAC;}
    E_Type_BT_Device getType() const {return bt_type;}
    bool begun() const {return _begun;}
    uint8_t readBatt() const {return u8_Batt;}
    void disconnect(uint8_t reason);
    void disconnect(uint16_t conn_handle, uint8_t reason);
    uint16_t getConnHandle(){return _conn_handle;};
    
    virtual void discover(uint16_t conn_handle) {};
    virtual bool discovered() { return false; };
    virtual void begin() {};
};

 #endif /* BT_DEVICE */
#ifndef cps_H
#define cps_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include "Utils.h"
#include <functional>

#include "BT_Device.h"

// Cycling Speed and Cadence configuration
#define     GATT_CPS_UUID                           0x1818
#define     GATT_CPS_MEASUREMENT_UUID               0x2A63
#define     GATT_CPS_VECTOR_UUID                    0x2A64
#define     GATT_CPS_FEATURE_UUID                   0x2A65
#define     GATT_CPS_CONTROL_POINT_UUID             0x2A66

class cps : public BT_Device {

  private:
    BLEClientService        cps_serv  = BLEClientService(GATT_CPS_UUID);
    BLEClientCharacteristic cps_meas  = BLEClientCharacteristic(GATT_CPS_MEASUREMENT_UUID);
    BLEClientCharacteristic cps_feat  = BLEClientCharacteristic(GATT_CPS_FEATURE_UUID);

    
    cps(){
      this->bt_type = E_Type_BT_Device::bt_cps;
    }

  protected:
    cps(String name, uint8_t* MAC){
      this->bt_type = E_Type_BT_Device::bt_cps;
      this->name = name;
      copyMAC(this->MAC, MAC);
      this->begin();
    }

  public:
    float f32_power;
    
    static void create_cps(String name, uint8_t* MAC)
    {
      btDevices.push_back(std::unique_ptr<cps>(new cps(name, MAC)));
    };

    static void cps_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    void begin();

    void discover(uint16_t conn_handle);

    void cps_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    
    bool discovered();
};

 #endif /* cps_H */
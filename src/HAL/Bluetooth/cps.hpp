#ifndef cps_H
#define cps_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include <functional>

#include "BT_Device.hpp"
#include "Utils.hpp"

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
    BLEClientCharacteristic cps_loc   = BLEClientCharacteristic(GATT_SENSOR_LOCATION_UUID);

    bool _CadencePresent, _TorquePresent, _BalancePresent, _ForceMagPresent;
    uint16_t u16_feature;
    uint8_t u8_location;

    cps(){
      this->bt_type = E_Type_BT_Device::bt_cps;
    }

  protected:
    cps(const char* name, size_t nameLen, MacAddress MAC){
      this->bt_type = E_Type_BT_Device::bt_cps;
      memcpy(this->name, name, nameLen);
      this->MAC = MAC;
      this->begin();
    }

  public:
    float f32_power, f32_cadence, f32_torque, f32_pedal_balance, f32_force_magnitude;
    virtual ~cps(){};
    
    static void create_cps(const char* name, size_t nameLen, MacAddress MAC)
    {
      btDevices.push_back(std::unique_ptr<cps>(new cps(name, nameLen, MAC)));
    };

    static void cps_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    static std::vector<float> getPower();
    static std::vector<float> getCadence();
    static std::vector<float> getTorque();
    static std::vector<float> getPedalBalance();
    static std::vector<float> getForceMagnitude();

    void begin();

    void discover(uint16_t conn_handle);

    void cps_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    
    bool discovered();
};

 #endif /* cps_H */
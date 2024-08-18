#ifndef hrm_H
#define hrm_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include "Utils.h"
#include <functional>

#include "BT_Device.h"

class hrm : public BT_Device {
  private:
    BLEClientService        hrm_serv = BLEClientService(UUID16_SVC_HEART_RATE);
    BLEClientCharacteristic hrm_meas = BLEClientCharacteristic(UUID16_CHR_HEART_RATE_MEASUREMENT);
    BLEClientCharacteristic hrm_loc  = BLEClientCharacteristic(UUID16_CHR_BODY_SENSOR_LOCATION);

  
    static void hrm_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    static void hrm_static_disconnect_callback(uint16_t conn_handle, uint8_t reason);

  protected:
    hrm(){this->type = E_Type_BT_Device::hrm;};

  public:
    uint16_t u16_bpm;
    hrm(String name, uint8_t* MAC){
      hrm();
      this->name = name;
      copyMAC(this->MAC, MAC);
    };
    
    static void create_hrm(String name, uint8_t* MAC)
    {
      btDevices.emplace_back(std::unique_ptr<hrm>(new hrm(name, MAC)));
    };

    void hrm_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    void disconnect(uint16_t conn_handle, uint8_t reason);
    void discover(uint16_t conn_handle);
    bool discovered(){return hrm_meas.discovered();}
    void begin();
};
#endif
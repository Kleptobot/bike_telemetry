#ifndef hrm_H
#define hrm_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include <functional>

#include "BT_Device.hpp"

class hrm : public BT_Device {
private:
  BLEClientService hrm_serv = BLEClientService(UUID16_SVC_HEART_RATE);
  BLEClientCharacteristic hrm_meas = BLEClientCharacteristic(UUID16_CHR_HEART_RATE_MEASUREMENT);
  BLEClientCharacteristic hrm_loc = BLEClientCharacteristic(UUID16_CHR_BODY_SENSOR_LOCATION);


  static void hrm_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

  static void hrm_static_disconnect_callback(uint16_t conn_handle, uint8_t reason);
  hrm() {
    this->bt_type = E_Type_BT_Device::bt_hrm;
  }; 

protected:
  hrm(MacAddress MAC) {
    this->bt_type = E_Type_BT_Device::bt_hrm;
    this->MAC=MAC;
    this->begin();
  };

public:
  uint16_t u16_bpm=0;
  float f32_bpm=0;
  virtual ~hrm(){};

  static void create_hrm(MacAddress MAC) {
    btDevices.emplace_back(std::unique_ptr<hrm>(new hrm(MAC)));
  };

  static std::vector<float> getHRM();

  void hrm_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
  void disconnect(uint16_t conn_handle, uint8_t reason);
  void discover(uint16_t conn_handle);
  bool discovered() override {return hrm_meas.discovered();};
  void begin();

  void update(uint32_t now) override {
    f32_bpm = 0.8 * f32_bpm + 0.2 * (float)u16_bpm;
  }
};
#endif
#ifndef HRM_H
#define HRM_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include "HAL/SensorData.hpp"

#include "BT_Device.hpp"

class hrm : public BT_Device {
private:
  BLEClientService hrm_serv = BLEClientService(UUID16_SVC_HEART_RATE);
  BLEClientCharacteristic hrm_meas = BLEClientCharacteristic(UUID16_CHR_HEART_RATE_MEASUREMENT);
  BLEClientCharacteristic hrm_loc = BLEClientCharacteristic(UUID16_CHR_BODY_SENSOR_LOCATION);
  uint16_t u16_bpm=0;
  float f32_bpm=0;
  bool _hasData=false;   // set once a measurement notification has been decoded

  static std::vector<hrm*> _hrmDevices;

  static void hrm_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
  
  hrm() {
    this->bt_type = E_Type_BT_Device::bt_hrm;
  }; 

  void removeFromLocalList() override {
    _hrmDevices.erase(
      std::remove(_hrmDevices.begin(), _hrmDevices.end(), this),
      _hrmDevices.end()
    );
  }

protected:
  hrm(MacAddress MAC) {
    this->bt_type = E_Type_BT_Device::bt_hrm;
    this->MAC=MAC;
    this->begin();
  };

public:
  virtual ~hrm(){};

  static void create_hrm(MacAddress MAC) {
    btDevices.emplace_back(std::unique_ptr<hrm>(new hrm(MAC)));
    _hrmDevices.push_back(static_cast<hrm*>(btDevices.back().get()));
  };

  static data_record getHRM();

  bool hasData() const { return _hasData; }

    void begin();

    void discover(uint16_t conn_handle);

    void hrm_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    
    bool discovered() override;

    void disconnect(uint16_t conn_handle, uint8_t reason) override;

    void update(uint32_t now) override;
};
#endif /* HRM_H */
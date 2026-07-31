#ifndef CPS_H
#define CPS_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>

#include "BT_Device.hpp"
#include "HAL/SensorData.hpp"

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

    static std::vector<cps*> _cpsDevices;

    // In-class initialisers: cps objects are heap-allocated via `new cps(MAC)`,
    // so unlike the HAL/App singletons they get no static zero-init. getPower()
    // is called from HAL::update() on every main-loop iteration starting at
    // BOOT -- before any notification has arrived -- so these must not be
    // indeterminate.
    bool _CadencePresent = false;
    bool _TorquePresent = false;
    bool _BalancePresent = false;
    bool _ForceMagPresent = false;
    bool _hasData = false;          // set once a measurement has been decoded
    bool _hasCrankData = false;     // set once a crank revolution pair is known
    uint16_t u16_feature = 0;
    uint8_t u8_location = 0;
    uint16_t u16_CrankRevs_Prev = 0;
    uint16_t u16_CrankEvt_Prev = 0;

    cps(){
      this->bt_type = E_Type_BT_Device::bt_cps;
    }

    void removeFromLocalList() override {
      _cpsDevices.erase(
        std::remove(_cpsDevices.begin(), _cpsDevices.end(), this),
        _cpsDevices.end()
      );
    }

  protected:
    cps(MacAddress MAC){
      this->bt_type = E_Type_BT_Device::bt_cps;
      this->MAC = MAC;
      this->begin();
    }

  public:
    float f32_power = 0.0f;
    float f32_cadence = 0.0f;
    float f32_torque = 0.0f;
    float f32_pedal_balance = 0.0f;
    float f32_force_magnitude = 0.0f;

    // True once at least one measurement notification has been decoded.
    // getPower() must not report a reading before this.
    bool hasData() const { return _hasData; }

    virtual ~cps(){};
    
    static void create_cps(MacAddress MAC)
    {
      btDevices.push_back(std::unique_ptr<cps>(new cps(MAC)));
      _cpsDevices.push_back(static_cast<cps*>(btDevices.back().get()));
    };

    static void cps_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    static data_record getPower();
    static data_record getCadence();
    static data_record getTorque();
    static data_record getPedalBalance();
    static data_record getForceMagnitude();

    void begin();

    void discover(uint16_t conn_handle);

    void cps_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    
    bool discovered() override;

    void update(uint32_t now) override {}
    
    void disconnect(uint16_t conn_handle, uint8_t reason) override {
      (void) conn_handle;
      (void) reason;
    
      BT_Device::disconnect(conn_handle, reason);
    };
};

 #endif /* CPS_H */
#ifndef csc_H
#define csc_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include <functional>

#include "BT_Device.hpp"

// Cycling Speed and Cadence configuration
#define     GATT_CSC_UUID                           0x1816
#define     GATT_CSC_MEASUREMENT_UUID               0x2A5B
#define     GATT_CSC_FEATURE_UUID                   0x2A5C
#define     GATT_SC_CONTROL_POINT_UUID              0x2A55

/* Device Information configuration */
#define     GATT_DEVICE_INFO_UUID                   0x180A
#define     GATT_MANUFACTURER_NAME_UUID             0x2A29
#define     GATT_MODEL_NUMBER_UUID                  0x2A24

/*CSC Measurement flags*/
#define     CSC_MEASUREMENT_WHEEL_REV_PRESENT       0x01
#define     CSC_MEASUREMENT_CRANK_REV_PRESENT       0x02

/* CSC feature flags */
#define     CSC_FEATURE_WHEEL_REV_DATA              0x01
#define     CSC_FEATURE_CRANK_REV_DATA              0x02
#define     CSC_FEATURE_MULTIPLE_SENSOR_LOC         0x04

/* SC Control Point op codes */
#define     SC_CP_OP_SET_CUMULATIVE_VALUE           1
#define     SC_CP_OP_START_SENSOR_CALIBRATION       2
#define     SC_CP_OP_UPDATE_SENSOR_LOCATION         3
#define     SC_CP_OP_REQ_SUPPORTED_SENSOR_LOCATIONS 4
#define     SC_CP_OP_RESPONSE                       16

/*SC Control Point response values */
#define     SC_CP_RESPONSE_SUCCESS                  1
#define     SC_CP_RESPONSE_OP_NOT_SUPPORTED         2
#define     SC_CP_RESPONSE_INVALID_PARAM            3
#define     SC_CP_RESPONSE_OP_FAILED                4

class csc : public BT_Device {

  private:
    BLEClientService        csc_serv  = BLEClientService(GATT_CSC_UUID);
    BLEClientCharacteristic csc_meas  = BLEClientCharacteristic(GATT_CSC_MEASUREMENT_UUID);
    BLEClientCharacteristic csc_feat  = BLEClientCharacteristic(GATT_CSC_FEATURE_UUID);
    BLEClientCharacteristic csc_loc   = BLEClientCharacteristic(GATT_SENSOR_LOCATION_UUID);

    // Body sensor location value is 8 bit
    const char* feat_str[4] = {"other", "Speed", "Cadence", "Speed & Cadence"};
    const float f32_circ = 2127;
    uint32_t u32_WheelCount_Prev;
    uint16_t u16_SpeedEvt_Prev, u16_CrankCount_Prev, u16_CrankEvt_Prev;
    uint8_t u8_feature, u8_location;
    uint32_t millis_at_spd_evt = 0, millis_at_cad_evt = 0;
    uint32_t exp_next_spd_evt= 0, exp_next_cad_evt=0;
    uint32_t u32_WheelCount_delta = 0;
    uint16_t u16_CrankCount_delta = 0, u16_speed_delta = 0, u16_crank_delta = 0;
    
    csc(){
      this->bt_type = E_Type_BT_Device::bt_csc;
    }

  protected:
    csc(MacAddress MAC){
      this->bt_type = E_Type_BT_Device::bt_csc;
      this->MAC=MAC;
      this->begin();
    }

  public:
    bool b_speed_present, b_cadence_present;
    float f32_kph_raw, f32_kph, f32_cadence_raw, f32_cadence;
    virtual ~csc(){};
    
    static void create_csc(MacAddress MAC)
    {
      btDevices.push_back(std::unique_ptr<csc>(new csc(MAC)));
    };

    static void csc_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    static std::vector<float> getSpeed();
    static std::vector<float> getCadence();

    void begin();

    void discover(uint16_t conn_handle);

    void csc_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    
    bool discovered() override;

    void disconnect(uint16_t conn_handle, uint8_t reason) override;

    void update(uint32_t now) override {
      float f32_kph_est = 0, f32_cad_est = 0;
      
      //use the last known u16_CrankCount_delta delta and an adjusted time delta to come up with an actual cadance estimate
      if (now>exp_next_cad_evt) {
        uint32_t diff = now - exp_next_cad_evt;
        uint32_t delta_adjusted = u16_crank_delta + diff * 0.9765625; //convert milliseconds into 1/1024ths of a second
        if ( delta_adjusted > 0)
          f32_cad_est = 61140.0 *float(u16_CrankCount_delta)/float(delta_adjusted);
      }else{
        f32_cad_est = f32_cadence_raw;
      }
      
      //low pass filter the cadance estimate
      f32_cadence = f32_cad_est * 0.05 + f32_cadence * 0.95;

      //use the last known wheelcount delta and an adjusted time delta to come up with an actual speed estimate
      if(now>exp_next_spd_evt) {
        uint32_t diff = now - exp_next_spd_evt;
        uint32_t delta_adjusted = u16_speed_delta + diff * 0.9765625;
        if ( delta_adjusted > 0)
          f32_kph_est = f32_circ * 3.6684 *float(u32_WheelCount_delta)/float(delta_adjusted);
      }else{
        f32_kph_est = f32_kph_raw;
      }
      
      //low pass filter the speed estimate
      f32_kph = f32_kph_est * 0.05 + f32_kph * 0.95;
    };
};

 #endif /* CSC_H */
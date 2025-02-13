#ifndef csc_H
#define csc_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <bluefruit.h>
#include "Utils.h"
#include <functional>

#include "BT_Device.h"

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
    uint32_t u32_WheelCount_Prev;
    uint16_t u16_SpeedEvt_Prev, u16_CrankCount_Prev, u16_CrankEvt_Prev;
    uint8_t u8_feature, u8_location;
    
    csc(){
      this->bt_type = E_Type_BT_Device::bt_csc;
    }

  protected:
    csc(char* name, size_t nameLen, uint8_t* MAC){
      this->bt_type = E_Type_BT_Device::bt_csc;
      memcpy(this->name, name, nameLen);
      copyMAC(this->MAC, MAC);
      this->begin();
    }

  public:
    bool b_speed_present, b_cadence_present;
    float f32_rpm, f32_kph, f32_cadence;
    virtual ~csc(){};
    
    static void create_csc(char* name, size_t nameLen, uint8_t* MAC)
    {
      btDevices.push_back(std::unique_ptr<csc>(new csc(name, nameLen, MAC)));
    };

    static void csc_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    static std::vector<float> getSpeed();
    static std::vector<float> getCadence();

    void begin();

    void discover(uint16_t conn_handle);

    void csc_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    
    bool discovered();
};

 #endif /* CSC_H */
#ifndef csc_H
#define csc_H

#include <Arduino.h>
#include <bluefruit.h>
#include "Utils.h"
#include <functional>

// Cycling Speed and Cadence configuration
#define     GATT_CSC_UUID                           0x1816
#define     GATT_CPS_UUID                           0x1818
#define     GATT_BAT_UUID                           0x180F
#define     GATT_BAT_MEASUREMENT_UUID               0x2A19
#define     GATT_CSC_MEASUREMENT_UUID               0x2A5B
#define     GATT_CSC_FEATURE_UUID                   0x2A5C
#define     GATT_SENSOR_LOCATION_UUID               0x2A5D
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

#define     MAX_CSC                                 2

//typedef void (*csc_notify_callback) (BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
//typedef void (*bat_notify_callback) (BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

class csc;

class csc {

  private:
    BLEClientService        csc_serv  = BLEClientService(GATT_CSC_UUID);
    BLEClientCharacteristic csc_meas  = BLEClientCharacteristic(GATT_CSC_MEASUREMENT_UUID);
    BLEClientCharacteristic csc_feat  = BLEClientCharacteristic(GATT_CSC_FEATURE_UUID);
    BLEClientCharacteristic csc_loc   = BLEClientCharacteristic(GATT_SENSOR_LOCATION_UUID);
    BLEClientService        bat_serv  = BLEClientService(GATT_BAT_UUID);
    BLEClientCharacteristic bat_meas  = BLEClientCharacteristic(GATT_BAT_MEASUREMENT_UUID);

    // Body sensor location value is 8 bit
    const char* feat_str[4] = {"other", "Speed", "Cadence", "Speed & Cadence"};
    uint32_t u32_WheelCount_Prev;
    uint16_t u16_SpeedEvt_Prev, u16_CrankCount_Prev, u16_CrankEvt_Prev;
    uint8_t u8_feature, u8_location, u8_batt=100;
    String name;
    uint8_t MAC[6];
    bool _begun;

    static csc* instantiated[];
    static int instances;



  public:

    bool b_speed_present, b_cadence_present;
    float f32_rpm, f32_kph, f32_cadence;
    
    csc()
    {
      instantiated[instances]=this;
      instances++;
    };

    csc(String name, uint8_t* MAC){
      csc();
      this->name = name;
      this->MAC[0] = MAC[0];
      this->MAC[1] = MAC[1];
      this->MAC[2] = MAC[2];
      this->MAC[3] = MAC[3];
      this->MAC[4] = MAC[4];
      this->MAC[5] = MAC[5];
    }

    static void csc_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
    
    static void bat_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    static void csc_static_disconnect_callback(uint16_t conn_handle, uint8_t reason);

    static void clearInstances();


    static csc* getDeviceWithMAC(uint8_t* MAC);

    void begin();

    void csc_discover(uint16_t conn_handle);

    void csc_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    void bat_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

    uint8_t *getMac(){return MAC;}
    String getName(){return (String)name;}

    bool begun(){return _begun;}
    
    bool discovered();

    void disconnect(uint16_t conn_handle, uint8_t reason);

    uint8_t readBatt();
};

 #endif /* CSC_H */
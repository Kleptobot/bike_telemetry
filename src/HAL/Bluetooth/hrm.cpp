#include "hrm.hpp"

std::vector<hrm*> hrm::_hrmDevices;

void hrm::hrm_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  //itterate the members of the bt device base
  for (hrm* dev : _hrmDevices) {
    //compare the conn handle of the evt with the conn handle of the device servic (static cast to an hrm safe because we know the type)
    if (chr->connHandle() == dev->hrm_serv.connHandle()) {
      //call the underlying notify method for the instance (again static cast)
      dev->hrm_notify(chr, data, len);
      return;
    }
  }
}

void hrm::hrm_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  if ( data[0] & bit(0) ) {
    memcpy(&u16_bpm, data+1, 2);
  } else {
    u16_bpm = data[1];
  }
  
  if (ENABLE_BLUETOOTH_DEBUG) {
    Serial.print("Received HRM Measurement: ");
    Serial.print(u16_bpm);
    Serial.println(" bpm");
  }
}

void hrm::begin() {
  // Initialize HRM client
  hrm_serv.begin();

  // Initialize client characteristics of HRM.
  // Note: Client Char will be added to the last service that is begin()ed.
  hrm_loc.begin();

  // set up callback for receiving measurement
  hrm_meas.setNotifyCallback(hrm_notify_callback);
  hrm_meas.begin();

  bat_serv.begin();

  bat_meas.setNotifyCallback(bat_notify_callback);
  bat_meas.begin();

  _begun = true;

  return;
}

bool hrm::discovered() {
  return hrm_meas.discovered();
}

void hrm::discover(uint16_t conn_handle) {
  Serial.println("Connected");
  Serial.print("Discovering HRM Service ... ");

  // If HRM is not found, disconnect and return
  if ( !hrm_serv.discover(conn_handle) )
  {
    Serial.println("Found NONE");

    // disconnect since we couldn't find HRM service
    Bluefruit.disconnect(conn_handle);

    return;
  }

  _conn_handle = conn_handle;

  // Once HRM service is found, we continue to discover its characteristic
  Serial.println("Found it");
  
  Serial.print("Discovering Measurement characteristic ... ");
  if ( !hrm_meas.discover() )
  {
    // Measurement chr is mandatory, if it is not found (valid), then disconnect
    Serial.println("not found !!!");  
    Serial.println("Measurement characteristic is mandatory but not found");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  Serial.println("Found it");

  // Measurement is found, continue to look for option Body Sensor Location
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.body_sensor_location.xml
  // Body Sensor Location is optional, print out the location in text if present
  Serial.print("Discovering Body Sensor Location characteristic ... ");
  if ( hrm_loc.discover() )
  {
    Serial.println("Found it");
    
    // Body sensor location value is 8 bit
    const char* body_str[] = { "Other", "Chest", "Wrist", "Finger", "Hand", "Ear Lobe", "Foot" };

    // Read 8-bit hrm_loc value from peripheral
    uint8_t loc_value = hrm_loc.read8();
    
    Serial.print("Body Location Sensor: ");
    Serial.println(body_str[loc_value]);
  }else
  {
    Serial.println("Found NONE");
  }

  // Reaching here means we are ready to go, let's enable notification on measurement chr
  if ( hrm_meas.enableNotify() )
  {
    Serial.println("Ready to receive HRM Measurement value");
  }else
  {
    Serial.println("Couldn't enable notify for HRM Measurement. Increase DEBUG LEVEL for troubleshooting");
  }
  if(bat_serv.discover(conn_handle))
  {
    Serial.println("Found bat");
    
    if (bat_meas.discover() )
    {
      u8_Batt = bat_meas.read8();
      if (ENABLE_BLUETOOTH_DEBUG) {
        Serial.print("Batt: "); Serial.println(u8_Batt);
      }
    }
    if ( bat_meas.enableNotify() )
    {
      Serial.println("Ready to receive BAT Measurement value");
    }else
    {
      Serial.println("Couldn't enable notify for BAT Measurement");
    }
  }
  f32_bpm=0;
}

data_record hrm::getHRM() {
  data_record heartrates = {0, false};
  for (hrm* dev : _hrmDevices) {
    heartrates.value += dev->f32_bpm;
    heartrates.live = true;
  }
  if (_hrmDevices.size() > 0) {
    heartrates.value /= _hrmDevices.size();
  }
  return heartrates;
}

void hrm::disconnect(uint16_t conn_handle, uint8_t reason) {
  (void)reason;
  (void)conn_handle;

  f32_bpm = 0;
  Serial.println("HRM Disconnected");

  // Call the base class disconnect method to handle any additional cleanup
  BT_Device::disconnect(conn_handle, reason);
}

void hrm::update(uint32_t now) {
  f32_bpm = 0.999f * f32_bpm + 0.001f * (float)u16_bpm;
  if (ENABLE_BLUETOOTH_DEBUG) {
    Serial.print("Smoothed BPM: ");
    Serial.println(f32_bpm);
  }
}
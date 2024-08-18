#include "hrm.h"

void hrm::hrm_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  //itterate the members of the bt device base
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    //check the type of the member
    if((*it).get()->getType() == E_Type_BT_Device::hrm)
    {
      //compare the conn handle of the evt with the conn handle of the device servic (static cast to an hrm safe because we know the type)
      if (chr->connHandle() == static_cast<hrm*>((*it).get())->hrm_serv.connHandle())
      {
        //call the underlying notify method for the instance (again static cast)
        static_cast<hrm*>((*it).get())->hrm_notify(chr, data, len);
        return;
      }
    }
  }
}

void hrm::hrm_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  if ( data[0] & bit(0) )
  {
    memcpy(&u16_bpm, data+1, 2);
  }
  else
  {
    u16_bpm = data[1];
  }
}

void hrm::begin()
{
  // Initialize HRM client
  hrm_serv.begin();

  // Initialize client characteristics of HRM.
  // Note: Client Char will be added to the last service that is begin()ed.
  hrm_loc.begin();

  // set up callback for receiving measurement
  hrm_meas.setNotifyCallback(hrm_notify_callback);
  hrm_meas.begin();

  _begun = true;

  return;
}

void hrm::discover(uint16_t conn_handle)
{
  logInfoln("Connected");
  logInfo("Discovering HRM Service ... ");

  // If HRM is not found, disconnect and return
  if ( !hrm_serv.discover(conn_handle) )
  {
    logInfoln("Found NONE");

    // disconnect since we couldn't find HRM service
    Bluefruit.disconnect(conn_handle);

    return;
  }

  // Once HRM service is found, we continue to discover its characteristic
  logInfoln("Found it");
  
  logInfo("Discovering Measurement characteristic ... ");
  if ( !hrm_meas.discover() )
  {
    // Measurement chr is mandatory, if it is not found (valid), then disconnect
    logInfoln("not found !!!");  
    logInfoln("Measurement characteristic is mandatory but not found");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  logInfoln("Found it");

  // Measurement is found, continue to look for option Body Sensor Location
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.body_sensor_location.xml
  // Body Sensor Location is optional, print out the location in text if present
  logInfo("Discovering Body Sensor Location characteristic ... ");
  if ( hrm_loc.discover() )
  {
    logInfoln("Found it");
    
    // Body sensor location value is 8 bit
    const char* body_str[] = { "Other", "Chest", "Wrist", "Finger", "Hand", "Ear Lobe", "Foot" };

    // Read 8-bit hrm_loc value from peripheral
    uint8_t loc_value = hrm_loc.read8();
    
    logInfo("Body Location Sensor: ");
    logInfoln(body_str[loc_value]);
  }else
  {
    logInfoln("Found NONE");
  }

  // Reaching here means we are ready to go, let's enable notification on measurement chr
  if ( hrm_meas.enableNotify() )
  {
    logInfoln("Ready to receive HRM Measurement value");
  }else
  {
    logInfoln("Couldn't enable notify for HRM Measurement. Increase DEBUG LEVEL for troubleshooting");
  }
}
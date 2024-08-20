#include <iterator>
#include "cps.h"

void cps::cps_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  //itterate the members of the bt device base
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    //check the type of the member
    if((*it)->getType() == E_Type_BT_Device::bt_cps)
    {
      //compare the conn handle of the evt with the conn handle of the device servic (static cast to an hrm safe because we know the type)
      if (chr->connHandle() == static_cast<cps*>((*it).get())->cps_serv.connHandle())
      {
        //call the underlying notify method for the instance (again static cast)
        static_cast<cps*>((*it).get())->cps_notify(chr, data, len);
        return;
      }
    }
  }
}

void cps::begin()
{
  // Initialize cps client
  cps_serv.begin();

  // Initialize client characteristics of cps.
  // Note: Client Char will be added to the last service that is begin()ed.
  cps_feat.begin();

  // set up callback for receiving measurement
  cps_meas.setNotifyCallback(cps_notify_callback);
  cps_meas.begin();

  bat_serv.begin();

  bat_meas.setNotifyCallback(bat_notify_callback);
  bat_meas.begin();

  _begun = true;
  return;
};

void cps::discover(uint16_t conn_handle)
{
  // If cps is not found, disconnect and return
  if (cps_serv.discover(conn_handle) )
  {
    _conn_handle = conn_handle;
    logInfoln("Found cps");

    if ( !cps_meas.discover() )
    {
      // Measurement chr is mandatory, if it is not found (valid), then disconnect
      return;
    }

    if ( cps_feat.discover() )
    {
      // Read 8-bit BSLC value from peripheral
    }

    // Reaching here means we are ready to go, let's enable notification on measurement chr
    if ( cps_meas.enableNotify() )
    {
      logInfoln("Ready to receive cps Measurement value");
    }else{
      logInfoln("Couldn't enable notify for cps Measurement");
    }
    if(bat_serv.discover(conn_handle))
    {
      logInfoln("Found bat");
      
      if (bat_meas.discover() )
      {
        u8_Batt = bat_meas.read8();
        //Serial.print("Batt: "); logInfoln(u8_batt);
      }
      if ( bat_meas.enableNotify() )
      {
        logInfoln("Ready to receive BAT Measurement value");
      }else
      {
        logInfoln("Couldn't enable notify for BAT Measurement");
      }
    }
  }else{
    Bluefruit.disconnect(conn_handle);
    logInfoln("Found NONE");
  }
  return;
}

bool cps::discovered()
{
  return cps_meas.discovered();
}

void cps::cps_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.service.cycling_speed_and_cadence.xml
  
}
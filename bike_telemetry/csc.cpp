#include <iterator>
#include "csc.h"

void csc::csc_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  //itterate the members of the bt device base
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    //check the type of the member
    if((*it)->getType() == E_Type_BT_Device::bt_csc)
    {
      //compare the conn handle of the evt with the conn handle of the device servic (static cast to an hrm safe because we know the type)
      if (chr->connHandle() == static_cast<csc*>((*it).get())->csc_serv.connHandle())
      {
        //call the underlying notify method for the instance (again static cast)
        static_cast<csc*>((*it).get())->csc_notify(chr, data, len);
        return;
      }
    }
  }
}

void csc::begin()
{
  // Initialize csc client
  csc_serv.begin();

  // Initialize client characteristics of csc.
  // Note: Client Char will be added to the last service that is begin()ed.
  csc_loc.begin();
  csc_feat.begin();

  // set up callback for receiving measurement
  csc_meas.setNotifyCallback(csc_notify_callback);
  csc_meas.begin();

  bat_serv.begin();

  bat_meas.setNotifyCallback(bat_notify_callback);
  bat_meas.begin();

  b_cadence_present =0;
  b_speed_present =0;

  _begun = true;
  return;
};

void csc::discover(uint16_t conn_handle)
{
  // If csc is not found, disconnect and return
  if (csc_serv.discover(conn_handle) )
  {
    _conn_handle = conn_handle;
    Serial.println("Found CSC");

    if ( !csc_meas.discover() )
    {
      // Measurement chr is mandatory, if it is not found (valid), then disconnect
      return;
    }

    if ( csc_feat.discover() )
    {
      // Read 8-bit BSLC value from peripheral
      
      u32_WheelCount_Prev = 0;
      u16_SpeedEvt_Prev = 0;
      u16_CrankCount_Prev = 0;
      u16_CrankEvt_Prev = 0;
      f32_rpm = 0;
      f32_kph = 0;
      f32_cadence = 0;

      u8_feature = csc_feat.read8();
      b_speed_present = ((u8_feature & 0x01) == 1);
      b_cadence_present = ((u8_feature & 0x02) == 2);
    }
    
    if ( csc_loc.discover() )
    {
      // Read 8-bit BSLC value from peripheral
      u8_location = csc_loc.read8();
    }

    // Reaching here means we are ready to go, let's enable notification on measurement chr
    if ( csc_meas.enableNotify() )
    {
      Serial.println("Ready to receive CSC Measurement value");
    }else{
      Serial.println("Couldn't enable notify for CSC Measurement");
    }
    if(bat_serv.discover(conn_handle))
    {
      Serial.println("Found bat");
      
      if (bat_meas.discover() )
      {
        u8_Batt = bat_meas.read8();
        //Serial.print("Batt: "); Serial.println(u8_batt);
      }
      if ( bat_meas.enableNotify() )
      {
        Serial.println("Ready to receive BAT Measurement value");
      }else
      {
        Serial.println("Couldn't enable notify for BAT Measurement");
      }
    }
  }else{
    Bluefruit.disconnect(conn_handle);
    Serial.println("Found NONE");
  }
  return;
}

bool csc::discovered()
{
  return csc_meas.discovered();
}

void csc::csc_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.service.cycling_speed_and_cadence.xml
  
  uint8_t offset = 1;
  uint32_t u32_WheelCount, u32_WheelCount_delta;
  uint16_t u16_SpeedEvt, u16_CrankCount, u16_CrankCount_delta, u16_CrankEvt, u16_speed_delta, u16_crank_delta;
  float f32_cadence_raw;

  float f32_circ = 2127;

  if ((data[0] & 0x01) ==1)
  {
    memcpy(&u32_WheelCount, data+offset, 4);
    offset += 4;
    memcpy(&u16_SpeedEvt, data+offset, 2);
    offset += 2;

    //check for disconnect
    if(_disconnected)
    {
      u16_SpeedEvt_Prev=u16_SpeedEvt;
      u32_WheelCount_Prev=u32_WheelCount;
      u16_CrankEvt_Prev = u16_CrankEvt;
      u16_CrankCount_Prev = u16_CrankCount;
      _disconnected=false;
    }
  
    //check for overflow of speed evt
    if (u16_SpeedEvt>u16_SpeedEvt_Prev)
    {
      u16_speed_delta = u16_SpeedEvt - u16_SpeedEvt_Prev;
      u16_SpeedEvt_Prev = u16_SpeedEvt;
    }
    else if(u16_SpeedEvt_Prev>u16_SpeedEvt)
    {
      u16_speed_delta = u16_SpeedEvt + (65535 - u16_SpeedEvt_Prev);
      u16_SpeedEvt_Prev = u16_SpeedEvt;
    }
    else
    {
      u16_speed_delta = 0;
    }

     //check for overflow of speed count
    if (u32_WheelCount >= u32_WheelCount_Prev)
    {
      u32_WheelCount_delta = u32_WheelCount-u32_WheelCount_Prev;
    }
    else if(u32_WheelCount_Prev>u32_WheelCount)
    {
      u32_WheelCount_delta = u32_WheelCount + (4294967295 - u32_WheelCount_Prev);
    }
    u32_WheelCount_Prev = u32_WheelCount;

    if (u16_speed_delta > 0)
    {
      f32_rpm = 61140.0 *float(u32_WheelCount_delta)/float(u16_speed_delta);
    }else if (f32_rpm>200){
      f32_rpm -= 200;
    }else if (f32_rpm>100){
      f32_rpm -= 100;
    }else if (f32_rpm>50){
      f32_rpm -= 50;
    }else if (f32_rpm>20){
      f32_rpm -= 20;
    }else if (f32_rpm>5){
      f32_rpm -= 5;
    }else if (f32_rpm>1){
      f32_rpm -= 1;
    }else if (f32_rpm<1){
      if (f32_rpm >0.1)
        f32_rpm -= 0.1;
    }
    f32_kph = (f32_circ * 0.00006 * f32_rpm) * 0.3 + 0.7*f32_kph;
  }
  if ((data[0] & 0x02) == 2)
  {
    memcpy(&u16_CrankCount, data+offset, 2);
    offset += 2;
    memcpy(&u16_CrankEvt, data+offset, 2);
    offset += 2;

    //check for overflow of crank evt
    if (u16_CrankEvt>u16_CrankEvt_Prev)
    {
      u16_crank_delta = u16_CrankEvt - u16_CrankEvt_Prev;
      u16_CrankEvt_Prev = u16_CrankEvt;
    }
    else if(u16_CrankEvt_Prev>u16_CrankEvt)
    {
      u16_crank_delta = u16_CrankEvt + (65535 - u16_CrankEvt_Prev);
      u16_CrankEvt_Prev = u16_CrankEvt;
    }
    else
    {
      u16_crank_delta = 0;
    }

    //check for overflow of crank count
    if (u16_CrankCount>u16_CrankCount_Prev)
    {
      u16_CrankCount_delta = u16_CrankCount - u16_CrankCount_Prev;
      u16_CrankCount_Prev = u16_CrankCount;
    }
    else if(u16_CrankCount_Prev>u16_CrankCount)
    {
      u16_CrankCount_delta = u16_CrankCount + (65535 - u16_CrankCount_Prev);
      u16_CrankCount_Prev = u16_CrankCount;
    }
    else
    {
      u16_CrankCount_delta = 0;
    }
    
    if (u16_crank_delta>0)
    {
      f32_cadence_raw = 61140.0 *float(u16_CrankCount_delta)/float(u16_crank_delta);
    }else if (f32_cadence_raw>200){
      f32_cadence_raw -= 200;
    }else if (f32_cadence_raw>100){
      f32_cadence_raw -= 100;
    }else if (f32_cadence_raw>50){
      f32_cadence_raw -= 50;
    }else if (f32_cadence_raw>20){
      f32_cadence_raw -= 20;
    }else if (f32_cadence_raw>5){
      f32_cadence_raw -= 5;
    }else if (f32_cadence_raw>1){
      f32_cadence_raw -= 1;
    }else if (f32_cadence_raw<1){
      if (f32_cadence_raw >0.1)
      f32_cadence_raw -= 0.1;
    }
    f32_cadence = f32_cadence * 0.7 + f32_cadence_raw*0.3;
  }
}

std::vector<float> csc::getSpeed()
{
  std::vector<float> speeds;
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if((*it)->getType() == E_Type_BT_Device::bt_csc)
    {
      csc* temp_csc = static_cast<csc*>((*it).get());
      if(temp_csc->b_speed_present)
        speeds.push_back(temp_csc->f32_kph);
    }
  }
  return speeds;
}

std::vector<float> csc::getCadence()
{
  std::vector<float> cadences;
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if((*it)->getType() == E_Type_BT_Device::bt_csc)
    {
      csc* temp_csc = static_cast<csc*>((*it).get());
      if(temp_csc->b_cadence_present)
        cadences.push_back(temp_csc->f32_cadence);
    }
  }
  return cadences;
}
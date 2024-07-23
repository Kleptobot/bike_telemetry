#include "csc.h"

int csc::instances;
csc* csc::instantiated[MAX_CSC];

void csc::csc_static_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  for (int i=0; i <= csc::instances; i++)
  {
    if (chr->connHandle() == instantiated[i]->csc_serv.connHandle())
    {
      instantiated[i]->csc_notify_callback(chr, data, len);
      return;
    }
  }
}

void csc::csc_static_disconnect_callback(uint16_t conn_handle)
{
  for (int i=0; i < csc::instances; i++)
  {
    if (!instantiated[i]->discovered())
    {
      instantiated[i]->b_cadence_present =0;
      instantiated[i]->b_speed_present =0;
      return;
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
  csc_meas.setNotifyCallback(csc_static_callback);
  csc_meas.begin();

  b_cadence_present =0;
  b_speed_present =0;

  _begun = true;

  return;
};

void csc::csc_discover(uint16_t conn_handle)
{
  // If csc is not found, disconnect and return
  if ( !csc_serv.discover(conn_handle) )
  {
    return;
  }

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
    //Ready to receive CSC Measurement value
    return;
  }else{
    //Couldn't enable notify for csc Measurement. Increase DEBUG LEVEL for troubleshooting
    return;
  }
}

bool csc::discovered()
{
  return csc_meas.discovered();
}

void csc::csc_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
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
    
    f32_cadence_raw = f32_cadence;
    if (u16_crank_delta>0)
    {
      f32_cadence_raw = 61140.0 *float(u16_CrankCount_delta)/float(u16_crank_delta);
    }else if (f32_cadence>200){
      f32_cadence_raw = f32_cadence - 200;
    }else if (f32_cadence>100){
      f32_cadence_raw = f32_cadence - 100;
    }else if (f32_cadence>50){
      f32_cadence_raw = f32_cadence - 50;
    }else if (f32_cadence>20){
      f32_cadence_raw = f32_cadence - 20;
    }else if (f32_cadence>5){
      f32_cadence_raw = f32_cadence - 5;
    }else if (f32_cadence>1){
      f32_cadence_raw = f32_cadence - 1;
    }else if (f32_cadence<1){
      if (f32_cadence >0.1)
      f32_cadence_raw = f32_cadence - 0.1;
    }
  }

  f32_cadence = f32_cadence * 0.7 + f32_cadence_raw*0.3;

}
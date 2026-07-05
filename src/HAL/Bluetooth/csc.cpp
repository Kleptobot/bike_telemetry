#include <iterator>
#include "csc.hpp"

std::vector<csc*> csc::_cscDevices;

void csc::csc_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  //itterate the members of the bt device base
  for (csc* dev : _cscDevices) {
    //compare the conn handle of the evt with the conn handle of the device servic (static cast to a csc, safe because we know the type)
    if (chr->connHandle() == dev->csc_serv.connHandle() )
    {
      //call the underlying notify method for the instance (again static cast)
      dev->csc_notify(chr, data, len);
      return;
    }
  }
}

void csc::begin() {
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

void csc::discover(uint16_t conn_handle) {
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
      f32_kph_raw = 0;
      f32_cadence_raw = 0;
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

void csc::csc_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.service.cycling_speed_and_cadence.xml
  
  uint8_t offset = 1;
  uint32_t u32_WheelCount;
  uint16_t u16_SpeedEvt, u16_CrankCount, u16_CrankEvt;

  if ((data[0] & 0x01) ==1) {
    memcpy(&u32_WheelCount, data+offset, 4);
    offset += 4;
    memcpy(&u16_SpeedEvt, data+offset, 2);
    offset += 2;

    //check for disconnect
    if(_disconnected) {
      u16_SpeedEvt_Prev=u16_SpeedEvt;
      u32_WheelCount_Prev=u32_WheelCount;
      _disconnected=false;
    }
  
    //check for overflow of speed evt
    if (u16_SpeedEvt != u16_SpeedEvt_Prev) {
      u16_speed_delta = 0;
      if (u16_SpeedEvt>u16_SpeedEvt_Prev) {
        u16_speed_delta = u16_SpeedEvt - u16_SpeedEvt_Prev;
      } else {
        u16_speed_delta = u16_SpeedEvt + (65535 - u16_SpeedEvt_Prev);
      }
      u16_SpeedEvt_Prev = u16_SpeedEvt;
      millis_at_spd_evt = millis();
      exp_next_spd_evt = millis_at_spd_evt + uint(u16_speed_delta*1.024);
    }

     //check for overflow of speed count
    u32_WheelCount_delta=0;
    if (u32_WheelCount >= u32_WheelCount_Prev) {
      u32_WheelCount_delta = u32_WheelCount-u32_WheelCount_Prev;
    } else if(u32_WheelCount_Prev>u32_WheelCount) {
      u32_WheelCount_delta = u32_WheelCount + (4294967295 - u32_WheelCount_Prev);
    }
    u32_WheelCount_Prev = u32_WheelCount;

    //calculate the rate of change and convert to km/h
    f32_kph_raw = 0;
    if (u16_speed_delta > 0) {
      f32_kph_raw = f32_circ * 3.6684 *float(u32_WheelCount_delta)/float(u16_speed_delta);
    }
  }
  if ((data[0] & 0x02) == 2) {
    memcpy(&u16_CrankCount, data+offset, 2);
    offset += 2;
    memcpy(&u16_CrankEvt, data+offset, 2);
    offset += 2;

    //check for disconnect
    if(_disconnected) {
      u16_CrankEvt_Prev = u16_CrankEvt;
      u16_CrankCount_Prev = u16_CrankCount;
      _disconnected=false;
    }

    //check for overflow of crank evt
    if (u16_CrankEvt != u16_CrankEvt_Prev) {
      u16_crank_delta = 0;
      if (u16_CrankEvt>u16_CrankEvt_Prev) {
        u16_crank_delta = u16_CrankEvt - u16_CrankEvt_Prev;
      } else {
        u16_crank_delta = u16_CrankEvt + (65535 - u16_CrankEvt_Prev);
      }
      millis_at_cad_evt = millis();
      exp_next_cad_evt = millis_at_cad_evt + uint(u16_crank_delta/1024.0);
      u16_CrankEvt_Prev = u16_CrankEvt;
    }

    //check for overflow of crank count
    u16_CrankCount_delta = 0;
    if (u16_CrankCount>u16_CrankCount_Prev) {
      u16_CrankCount_delta = u16_CrankCount - u16_CrankCount_Prev;
    } else if(u16_CrankCount_Prev>u16_CrankCount) {
      u16_CrankCount_delta = u16_CrankCount + (65535 - u16_CrankCount_Prev);
    }
    u16_CrankCount_Prev = u16_CrankCount;
    
    //calculate rate of change and convert to rpm
    f32_cadence_raw = 0;
    if (u16_crank_delta>0) {
      f32_cadence_raw = 61140.0 *float(u16_CrankCount_delta)/float(u16_crank_delta);
    }
  }
}

data_record csc::getSpeed() {
  data_record speed = {0, false};
  for (csc* dev : _cscDevices) {
    if(dev->b_speed_present) {
      speed.value += dev->f32_kph;
      speed.live = true;
    }
  }
  if (_cscDevices.size() > 0) {
    speed.value /= _cscDevices.size();
  }
  return speed;
}

data_record csc::getCadence() {
  data_record cadence = {0, false};
  for (csc* dev : _cscDevices) {
    if(dev->b_cadence_present) {
      cadence.value += dev->f32_cadence;
      cadence.live = true;
    }
  }
  if (_cscDevices.size() > 0) {
    cadence.value /= _cscDevices.size();
  }
  return cadence;
}

void csc::disconnect(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;
  (void) reason;

  b_speed_present = false;
  b_cadence_present = false;

  BT_Device::disconnect(conn_handle,reason);
}

void csc::update(uint32_t now) {
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
  f32_cadence = f32_cad_est * 0.001 + f32_cadence * 0.999;

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
  f32_kph = f32_kph_est * 0.001 + f32_kph * 0.999;
};
#include <iterator>
#include "csc.hpp"

std::vector<csc*> csc::_cscDevices;
uint32_t csc::_nextSourceId = 0;

void csc::csc_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  if (!chr) return;
  const auto device = BT_Device::getDeviceWithConnectionHandle(chr->connHandle());
  if (device && device->getType() == E_Type_BT_Device::bt_csc) {
    static_cast<csc*>(device.get())->csc_notify(chr, data, len);
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
      u16_SpeedEvt_Last = 0;
      u16_CrankEvt_Last = 0;
      millis_at_spd_evt = 0;
      millis_at_cad_evt = 0;
      _hasWheelSample = false;
      _hasCrankSample = false;
      _wheelResync = true;
      _crankResync = true;
      _disconnected = false;

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

bool csc::discovered() {
  return csc_meas.discovered();
}

void csc::csc_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.service.cycling_speed_and_cadence.xml
  
  uint8_t offset = 1;
  uint32_t u32_WheelCount;
  uint16_t u16_SpeedEvt, u16_CrankCount, u16_CrankEvt;

  // The payload comes from the remote sensor, so every field must be bounds
  // checked against len before it is read. A short or malformed notification
  // previously read past the end of the buffer.
  if (len < 1) return;

  if ((data[0] & 0x01) ==1) {
    if (len < offset + 6u) return;      // uint32 revolutions + uint16 event time
    memcpy(&u32_WheelCount, data+offset, 4);
    offset += 4;
    memcpy(&u16_SpeedEvt, data+offset, 2);
    offset += 2;

    const uint32_t receivedAt = millis();
    _hasWheelSample = true;
    if (_wheelResync) {
      u16_SpeedEvt_Prev=u16_SpeedEvt;
      u32_WheelCount_Prev=u32_WheelCount;
      u16_SpeedEvt_Last = u16_SpeedEvt;
      millis_at_spd_evt = receivedAt;
      _wheelResync = false;
    } else {
      // Free-running counters wrap naturally under unsigned subtraction.
      const uint16_t eventDelta = (uint16_t)(u16_SpeedEvt - u16_SpeedEvt_Prev);
      const uint32_t revDelta = (uint32_t)(u32_WheelCount - u32_WheelCount_Prev);
      u16_SpeedEvt_Prev = u16_SpeedEvt;
      u32_WheelCount_Prev = u32_WheelCount;
      u16_SpeedEvt_Last = u16_SpeedEvt;
      u32_WheelCount_Total += revDelta;

      if (eventDelta > 0 || revDelta > 0) {
        millis_at_spd_evt = receivedAt;
      }
    }
  }
  if ((data[0] & 0x02) == 2) {
    if (len < offset + 4u) return;      // uint16 revolutions + uint16 event time
    memcpy(&u16_CrankCount, data+offset, 2);
    offset += 2;
    memcpy(&u16_CrankEvt, data+offset, 2);
    offset += 2;

    const uint32_t receivedAt = millis();
    _hasCrankSample = true;
    if (_crankResync) {
      u16_CrankEvt_Prev = u16_CrankEvt;
      u16_CrankCount_Prev = u16_CrankCount;
      u16_CrankEvt_Last = u16_CrankEvt;
      millis_at_cad_evt = receivedAt;
      _crankResync = false;
    } else {
      const uint16_t eventDelta = (uint16_t)(u16_CrankEvt - u16_CrankEvt_Prev);
      const uint16_t revDelta = (uint16_t)(u16_CrankCount - u16_CrankCount_Prev);
      u16_CrankEvt_Prev = u16_CrankEvt;
      u16_CrankCount_Prev = u16_CrankCount;
      u16_CrankEvt_Last = u16_CrankEvt;
      u32_CrankCount_Total += revDelta;

      if (eventDelta > 0 || revDelta > 0) {
        millis_at_cad_evt = receivedAt;
      }
    }
  }
}

const csc* csc::latestWheelSource() {
  const csc* best = nullptr;
  for (const csc* dev : _cscDevices) {
    if (!dev->b_speed_present) continue;
    if (!best || dev->wheelEvtMillis() > best->wheelEvtMillis()) best = dev;
  }
  return best;
}

const csc* csc::latestCrankSource() {
  const csc* best = nullptr;
  for (const csc* dev : _cscDevices) {
    if (!dev->b_cadence_present) continue;
    if (!best || dev->crankEvtMillis() > best->crankEvtMillis()) best = dev;
  }
  return best;
}

void csc::disconnect(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;
  (void) reason;

  b_speed_present = false;
  b_cadence_present = false;
  _hasWheelSample = false;
  _hasCrankSample = false;
  _wheelResync = true;
  _crankResync = true;
  Serial.println("CSC Disconnected");

  BT_Device::disconnect(conn_handle,reason);
}

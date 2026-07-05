#include <iterator>
#include "cps.hpp"

std::vector<cps*> cps::_cpsDevices;

void cps::cps_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  //itterate the members of the bt device base
  for (cps* dev : _cpsDevices) {
    //compare the conn handle of the evt with the conn handle of the device servic (static cast to a cps safe because we know the type)
    if (chr->connHandle() == dev->cps_serv.connHandle()) {
      //call the underlying notify method for the instance (again static cast)
      dev->cps_notify(chr, data, len);
      return;
    }
  }
}

void cps::begin() {
  // Initialize cps client
  cps_serv.begin();

  // Initialize client characteristics of cps.
  // Note: Client Char will be added to the last service that is begin()ed.
  cps_feat.begin();

  // set up callback for receiving measurement
  cps_meas.setNotifyCallback(cps_notify_callback);
  cps_meas.begin();
  cps_loc.begin();

  bat_serv.begin();

  bat_meas.setNotifyCallback(bat_notify_callback);
  bat_meas.begin();

  _begun = true;
  return;
};


void cps::discover(uint16_t conn_handle) {
  if (cps_serv.discover(conn_handle))
  {
    _conn_handle = conn_handle;
    Serial.println("Found CPS");
    
    if (!cps_meas.discover()) return;
    if (cps_feat.discover()) {
      u16_feature = cps_feat.read16();
      Serial.print("CPS Features: "); Serial.println(u16_feature, HEX);
    }
    if (cps_loc.discover()) {
      u8_location = cps_loc.read8();
      Serial.print("Sensor Location: "); Serial.println(u8_location);
    }
    if (cps_meas.enableNotify()) {
      Serial.println("Ready to receive CPS Measurement value");
    }
    if (bat_serv.discover(conn_handle) && bat_meas.discover()) {
      u8_Batt = bat_meas.read8();
      bat_meas.enableNotify();
    }
  }
  else {
    Bluefruit.disconnect(conn_handle);
  }
}

bool cps::discovered() {
  return cps_meas.discovered();
}

void cps::cps_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  //https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.service.cycling_power.xml

  uint16_t power;
  uint16_t cadence = 0;
  int16_t torque = 0;
  int16_t pedal_balance = 0;
  int16_t force_magnitude = 0;
  uint8_t offset = 1;

  _CadencePresent = false;
  _TorquePresent = false;
  _BalancePresent = false;
  _ForceMagPresent = false;

  memcpy(&power, data + offset, 2);
  offset += 2;
  
  if (data[0] & 0x02) {
    memcpy(&cadence, data + offset, 2);
    _CadencePresent = true;
    offset += 2;
  }
  
  if (data[0] & 0x04) {
    memcpy(&torque, data + offset, 2);
    _TorquePresent = true;
    offset += 2;
  }
  
  if (data[0] & 0x08) {
    memcpy(&pedal_balance, data + offset, 2);
    _BalancePresent = true;
    offset += 2;
  }
  
  if (data[0] & 0x10) {
    memcpy(&force_magnitude, data + offset, 2);
    _ForceMagPresent = true;
    offset += 2;
  }
  
  f32_power = static_cast<float>(power);
  f32_cadence = static_cast<float>(cadence);
  f32_torque = static_cast<float>(torque) / 32.0; // Convert to Nm
  f32_pedal_balance = static_cast<float>(pedal_balance) / 100.0; // Convert to %
  f32_force_magnitude = static_cast<float>(force_magnitude) / 10.0; // Convert to Nm
  
  Serial.print("Power: "); Serial.println(f32_power);
  Serial.print("Cadence: "); Serial.println(f32_cadence);
  Serial.print("Torque: "); Serial.println(f32_torque);
  Serial.print("Pedal Balance: "); Serial.println(f32_pedal_balance);
  Serial.print("Force Magnitude: "); Serial.println(f32_force_magnitude);
}

data_record cps::getPower() {
  data_record power = {0, false};
  for (cps* dev : _cpsDevices) {
    power.value += dev->f32_power;
    power.live = true;
  }
  if (_cpsDevices.size() > 0) {
    power.value /= _cpsDevices.size();
  }
  return power;
}

data_record cps::getCadence() {
  data_record cadences = {0, false};
  for (cps* dev : _cpsDevices) {
    if(dev->_CadencePresent) {
      cadences.value += dev->f32_cadence;
      cadences.live = true;
    }
  }
  if (_cpsDevices.size() > 0) {
    cadences.value /= _cpsDevices.size();
  }
  return cadences;
}

data_record cps::getTorque() {
  data_record torque_values = {0, false};
  for (cps* dev : _cpsDevices) {
    if(dev->_TorquePresent) {
      torque_values.value += dev->f32_torque;
      torque_values.live = true;
    }
  }
  if (_cpsDevices.size() > 0) {
    torque_values.value /= _cpsDevices.size();
  }
  return torque_values;
}

data_record cps::getPedalBalance() {
  data_record pedal_balance_values = {0, false};
  for (cps* dev : _cpsDevices) {
    if(dev->_BalancePresent) {
      pedal_balance_values.value += dev->f32_pedal_balance;
      pedal_balance_values.live = true;
    }
  }
  if (_cpsDevices.size() > 0) {
    pedal_balance_values.value /= _cpsDevices.size();
  }
  return pedal_balance_values;
}

data_record cps::getForceMagnitude() {
  data_record force_magnitude_values = {0, false};
  for (cps* dev : _cpsDevices) {
    if(dev->_ForceMagPresent) {
      force_magnitude_values.value += dev->f32_force_magnitude;
      force_magnitude_values.live = true;
    }
  }
  if (_cpsDevices.size() > 0) {
    force_magnitude_values.value /= _cpsDevices.size();
  }
  return force_magnitude_values;
}
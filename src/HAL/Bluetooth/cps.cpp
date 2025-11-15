#include <iterator>
#include "cps.hpp"

void cps::cps_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  //itterate the members of the bt device base
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    //check the type of the member
    if((*it)->getType() == E_Type_BT_Device::bt_cps)
    {
      //compare the conn handle of the evt with the conn handle of the device servic (static cast to a cps safe because we know the type)
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
  cps_loc.begin();

  bat_serv.begin();

  bat_meas.setNotifyCallback(bat_notify_callback);
  bat_meas.begin();

  _begun = true;
  return;
};


void cps::discover(uint16_t conn_handle)
{
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

bool cps::discovered()
{
  return cps_meas.discovered();
}

void cps::cps_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
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

std::vector<float> cps::getPower()
{
  std::vector<float> power_values;
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if((*it)->getType() == E_Type_BT_Device::bt_csc)
    {
      cps* temp_cps = static_cast<cps*>((*it).get());
      power_values.push_back(temp_cps->f32_power);
    }
  }
  return power_values;
}

std::vector<float> cps::getCadence()
{
  std::vector<float> cadences;
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if((*it)->getType() == E_Type_BT_Device::bt_csc)
    {
      cps* temp_cps = static_cast<cps*>((*it).get());
      if(temp_cps->_CadencePresent)
        cadences.push_back(temp_cps->f32_cadence);
    }
  }
  return cadences;
}

std::vector<float> cps::getTorque()
{
  std::vector<float> torque_values;
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if((*it)->getType() == E_Type_BT_Device::bt_csc)
    {
      cps* temp_cps = static_cast<cps*>((*it).get());
      if(temp_cps->_TorquePresent)
        torque_values.push_back(temp_cps->f32_torque);
    }
  }
  return torque_values;
}

std::vector<float> cps::getPedalBalance()
{
  std::vector<float> pedal_balance_values;
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if((*it)->getType() == E_Type_BT_Device::bt_csc)
    {
      cps* temp_cps = static_cast<cps*>((*it).get());
      if(temp_cps->_BalancePresent)
        pedal_balance_values.push_back(temp_cps->f32_pedal_balance);
    }
  }
  return pedal_balance_values;
}

std::vector<float> cps::getForceMagnitude()
{
  std::vector<float> force_magnitude_values;
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if((*it)->getType() == E_Type_BT_Device::bt_csc)
    {
      cps* temp_cps = static_cast<cps*>((*it).get());
      if(temp_cps->_ForceMagPresent)
        force_magnitude_values.push_back(temp_cps->f32_force_magnitude);
    }
  }
  return force_magnitude_values;
}
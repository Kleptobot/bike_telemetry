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
  (void)chr;
  // Cycling Power Measurement (0x2A63):
  //
  //   uint16  flags
  //   sint16  instantaneous power (watts)
  //   ... optional fields, in ascending flag-bit order
  //
  // The flags field is 16-bit and power starts at offset 2. The previous
  // implementation followed the CSC layout instead -- 8-bit flags, power at
  // offset 1 -- so power straddled the flags high byte and the power low byte,
  // and every optional field after it was shifted by one. The flag bits were
  // also misassigned: bit 1 is the balance *reference*, not cadence-present,
  // and cadence has to be derived from the crank revolution pair on bit 5.

  if (len < 4) return;                          // flags + power are mandatory

  const uint16_t flags = (uint16_t)data[0] | ((uint16_t)data[1] << 8);

  int16_t power;
  memcpy(&power, data + 2, 2);
  uint16_t offset = 4;

  _BalancePresent  = false;
  _TorquePresent   = false;
  _CadencePresent  = false;
  _ForceMagPresent = false;

  if (flags & CPS_FLAG_PEDAL_BALANCE_PRESENT) {
    if (len < offset + 1u) return;
    f32_pedal_balance = static_cast<float>(data[offset]) * 0.5f;   // 1/2 %
    _BalancePresent = true;
    offset += 1;
  }

  if (flags & CPS_FLAG_ACCUM_TORQUE_PRESENT) {
    if (len < offset + 2u) return;
    uint16_t torque;
    memcpy(&torque, data + offset, 2);
    f32_torque = static_cast<float>(torque) / 32.0f;               // 1/32 Nm
    _TorquePresent = true;
    offset += 2;
  }

  if (flags & CPS_FLAG_WHEEL_REV_PRESENT) {
    if (len < offset + 6u) return;
    offset += 6;   // uint32 cumulative + uint16 event time; speed comes from CSC
  }

  if (flags & CPS_FLAG_CRANK_REV_PRESENT) {
    if (len < offset + 4u) return;
    uint16_t revs, evt;
    memcpy(&revs, data + offset, 2);
    memcpy(&evt,  data + offset + 2, 2);
    offset += 4;

    // Cadence is a rate, not a transmitted value: revolutions per minute from
    // the change in cumulative revolutions over the change in event time,
    // which is expressed in 1/1024 s. Both counters are free-running uint16,
    // so plain unsigned subtraction handles wraparound.
    if (_hasCrankData) {
      uint16_t dRev = (uint16_t)(revs - u16_CrankRevs_Prev);
      uint16_t dEvt = (uint16_t)(evt  - u16_CrankEvt_Prev);
      if (dEvt > 0) {
        f32_cadence = 60.0f * 1024.0f * (float)dRev / (float)dEvt;
        _CadencePresent = true;
      }
    }
    u16_CrankRevs_Prev = revs;
    u16_CrankEvt_Prev  = evt;
    _hasCrankData = true;
  }

  if (flags & CPS_FLAG_EXTREME_FORCE_PRESENT) {
    if (len < offset + 4u) return;
    int16_t maxForce;
    memcpy(&maxForce, data + offset, 2);
    f32_force_magnitude = static_cast<float>(maxForce);            // newtons
    _ForceMagPresent = true;
    offset += 4;   // max and min
  }

  f32_power = static_cast<float>(power);
  _hasData = true;

  if (ENABLE_BLUETOOTH_DEBUG) {
    Serial.print("[CPS] power "); Serial.print(f32_power);
    Serial.print(" W, cadence "); Serial.print(f32_cadence);
    Serial.print(" rpm, torque "); Serial.print(f32_torque);
    Serial.print(" Nm, balance "); Serial.print(f32_pedal_balance);
    Serial.println(" %");
  }
}

data_record cps::getPower() {
  // Only report devices that have actually delivered a measurement. Without
  // the hasData() guard this reported f32_power for a device created at boot
  // from devices.txt but not yet connected.
  data_record power = {0, false};
  uint16_t contributors = 0;
  for (cps* dev : _cpsDevices) {
    if (dev->hasData()) {
      power.value += dev->f32_power;
      power.live = true;
      contributors++;
    }
  }
  if (contributors > 0) {
    power.value /= contributors;
  }
  return power;
}

data_record cps::getCadence() {
  data_record cadences = {0, false};
  uint16_t contributors = 0;
  for (cps* dev : _cpsDevices) {
    if(dev->_CadencePresent) {
      cadences.value += dev->f32_cadence;
      cadences.live = true;
      contributors++;
    }
  }
  if (contributors > 0) {
    cadences.value /= contributors;
  }
  return cadences;
}

data_record cps::getTorque() {
  data_record torque_values = {0, false};
  uint16_t contributors = 0;
  for (cps* dev : _cpsDevices) {
    if(dev->_TorquePresent) {
      torque_values.value += dev->f32_torque;
      torque_values.live = true;
      contributors++;
    }
  }
  if (contributors > 0) {
    torque_values.value /= contributors;
  }
  return torque_values;
}

data_record cps::getPedalBalance() {
  data_record pedal_balance_values = {0, false};
  uint16_t contributors = 0;
  for (cps* dev : _cpsDevices) {
    if(dev->_BalancePresent) {
      pedal_balance_values.value += dev->f32_pedal_balance;
      pedal_balance_values.live = true;
      contributors++;
    }
  }
  if (contributors > 0) {
    pedal_balance_values.value /= contributors;
  }
  return pedal_balance_values;
}

data_record cps::getForceMagnitude() {
  data_record force_magnitude_values = {0, false};
  uint16_t contributors = 0;
  for (cps* dev : _cpsDevices) {
    if(dev->_ForceMagPresent) {
      force_magnitude_values.value += dev->f32_force_magnitude;
      force_magnitude_values.live = true;
      contributors++;
    }
  }
  if (contributors > 0) {
    force_magnitude_values.value /= contributors;
  }
  return force_magnitude_values;
}
#include <memory>
#include "BT_Device.h"

std::vector<std::unique_ptr<BT_Device>> BT_Device::btDevices;

BT_Device* BT_Device::getDeviceWithMAC(uint8_t* MAC)
{
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if(compareMAC((*it)->getMac(),MAC))
      return (*it).get();
  }
  return NULL;
}

std::unique_ptr<BT_Device> BT_Device::removeDeviceWithMAC(uint8_t* MAC){
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if(compareMAC((*it)->getMac(),MAC)){
      auto retVal = std::move(*it);
      btDevices.erase(it);
      return std::move(retVal);
    }
  }
  return {};
}

std::unique_ptr<BT_Device> BT_Device::removeDevice(std::unique_ptr<BT_Device> device)
{
  auto it = std::find(btDevices.begin(), btDevices.end(), device); 
  //If element is found found, erase it 
  if (it != btDevices.end()) {
      auto retVal = std::move(*it);
      btDevices.erase(it);
      return std::move(retVal);
  }
  return {};
}

void BT_Device::disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if(conn_handle == (*it)->_conn_handle)
    {
      (*it)->disconnect(conn_handle, reason);
    }
  }
}

void BT_Device::disconnect(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
  
  logInfo("Disconnected, reason = 0x"); 
  logInfoln(String(reason, HEX));
}

void BT_Device::bat_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if (chr->connHandle() == (*it)->bat_serv.connHandle())
    {
      (*it)->bat_notify(chr, data, len);
      return;
    }
  }
  uint8_t bat_value = *data;
}

void BT_Device::bat_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  u8_Batt = *data;
}

bool BT_Device::all_devices_discovered()
{
  for (auto it = btDevices.begin(); it != btDevices.end(); it++)
  {
    if (!(*it)->discovered())
      return false;
  }
  return true;
}
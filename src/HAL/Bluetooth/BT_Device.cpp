#include <memory>
#include "BT_Device.hpp"

std::vector<std::unique_ptr<BT_Device>> BT_Device::btDevices;
BT_Device::DeviceDisconnectedCallback BT_Device::_onUnexpectedDisconnect;

BT_Device* BT_Device::getDeviceWithMAC(MacAddress MAC) {
    for (auto it = btDevices.begin(); it != btDevices.end(); it++) {
        if((*it)->getMac()==MAC)
            return (*it).get();
    }
    return NULL;
}

bool BT_Device::deviceWithMacDiscovered(MacAddress MAC) {
    for (auto it = btDevices.begin(); it != btDevices.end(); it++) {
        if((*it)->getMac()==MAC)
            return (*it)->discovered();
    }
    return false;
}

void BT_Device::removeDeviceWithMAC(MacAddress MAC) {
    Serial.println(btDevices.size());
    for (auto it = btDevices.begin(); it != btDevices.end(); it++) {
        if((*it)->getMac()==MAC) {
            // disconnect() removes this entry from btDevices itself, so we must
            // not touch `it` after calling it, and must not erase again here.
            (*it).get()->disconnect(22);
            Serial.println(btDevices.size());
            return;
        }
    }
    return;
}

void BT_Device::removeDevice(std::unique_ptr<BT_Device> device) {
    auto it = std::find(btDevices.begin(), btDevices.end(), device); 
    if (it != btDevices.end()) {
        (*it).get()->disconnect(22);
        btDevices.erase(it);
        return;
    }
    return;
}

void BT_Device::disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    for (auto it = btDevices.begin(); it != btDevices.end(); it++) {
        if (conn_handle == (*it)->_conn_handle) {
            (*it)->disconnect(conn_handle, reason);
            return;
        }
    }
}

void BT_Device::disconnect(uint16_t conn_handle, uint8_t reason) {
    (void) conn_handle;
    (void) reason;
    _disconnected = true;
    Bluefruit.disconnect(conn_handle);

    Serial.print("Disconnected, reason = 0x");
    Serial.println(String(reason, HEX));

    MacAddress macToRemove = MAC;

    // Fire the callback before erasing `this`, since `this` becomes invalid
    // as soon as the unique_ptr owning it is erased from btDevices below.
    if (_onUnexpectedDisconnect) {
        _onUnexpectedDisconnect(macToRemove);
    }
}

void BT_Device::disconnect(uint8_t reason) { disconnect(_conn_handle, reason); }

void BT_Device::bat_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  for (auto it = btDevices.begin(); it != btDevices.end(); it++) {
        if (chr->connHandle() == (*it)->bat_serv.connHandle()) {
            (*it)->bat_notify(chr, data, len);
            return;
        }
    }
}

void BT_Device::bat_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) { u8_Batt = *data; }

bool BT_Device::all_devices_discovered() {
    for (auto it = btDevices.begin(); it != btDevices.end(); it++) {
        if (!(*it)->discovered())
            return false;
    }
    return true;
}
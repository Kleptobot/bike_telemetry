#include <memory>
#include "BT_Device.hpp"

std::vector<std::shared_ptr<BT_Device>> BT_Device::btDevices;
BT_Device::DeviceDisconnectedCallback BT_Device::_onUnexpectedDisconnect;

std::shared_ptr<BT_Device> BT_Device::getDeviceWithMAC(MacAddress MAC) {
    BluetoothRegistryGuard guard;
    for (auto it = btDevices.begin(); it != btDevices.end(); it++) {
        if((*it)->getMac()==MAC)
            return *it;
    }
    return nullptr;
}

std::shared_ptr<BT_Device> BT_Device::getDeviceWithConnectionHandle(uint16_t connHandle) {
    BluetoothRegistryGuard guard;
    for (const auto& device : btDevices) {
        if (device->getConnHandle() == connHandle) return device;
    }
    return nullptr;
}

bool BT_Device::deviceWithMacDiscovered(MacAddress MAC) {
    const auto device = getDeviceWithMAC(MAC);
    return device && device->discovered();
}

void BT_Device::removeDeviceWithMAC(MacAddress MAC) {
    std::shared_ptr<BT_Device> removed;
    size_t remaining = 0;
    {
        BluetoothRegistryGuard guard;
        for (auto it = btDevices.begin(); it != btDevices.end(); it++) {
            if((*it)->getMac()==MAC) {
                removed = *it;
                removed->removeFromLocalList();
                btDevices.erase(it);
                break;
            }
        }
        remaining = btDevices.size();
    }
    Serial.println(remaining);
    if (removed) removed->disconnectForRemoval();
}

void BT_Device::removeDevice(std::shared_ptr<BT_Device> device) {
    if (!device) return;

    bool removed = false;
    {
        BluetoothRegistryGuard guard;
        auto it = std::find(btDevices.begin(), btDevices.end(), device);
        if (it != btDevices.end()) {
            (*it)->removeFromLocalList();
            btDevices.erase(it);
            removed = true;
        }
    }
    if (removed) device->disconnectForRemoval();
}

void BT_Device::disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    const auto device = getDeviceWithConnectionHandle(conn_handle);
    if (device) device->disconnect(conn_handle, reason);
}

void BT_Device::disconnectForRemoval() {
    _disconnected = true;
    if (_conn_handle != BLE_CONN_HANDLE_INVALID) {
        Bluefruit.disconnect(_conn_handle);
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

    // Queue UI-facing work instead of notifying the UI from the BLE task.
    if (_onUnexpectedDisconnect) {
        _onUnexpectedDisconnect(macToRemove);
    }
}

void BT_Device::disconnect(uint8_t reason) { disconnect(_conn_handle, reason); }

void BT_Device::bat_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
    if (!chr) return;
    const auto device = getDeviceWithConnectionHandle(chr->connHandle());
    if (device) device->bat_notify(chr, data, len);
}

void BT_Device::bat_notify(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) { u8_Batt = *data; }

bool BT_Device::all_devices_discovered() {
    BluetoothRegistryGuard guard;
    for (const auto& device : btDevices) {
        if (!device->discovered()) return false;
    }
    return true;
}
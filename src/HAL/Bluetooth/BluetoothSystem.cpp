#include "BluetoothSystem.hpp"

MacAddress BluetoothSystem::toConnectMAC;
std::vector<BluetoothDevice> BluetoothSystem::deviceList;
BluetoothSystem::DeviceListCallback BluetoothSystem::deviceListCallback;
E_Type_BT_Mode BluetoothSystem::_mode, BluetoothSystem::_mode_prev;
IStorage* BluetoothSystem::_storage;
uint32_t BluetoothSystem::lastBatUpdate = millis() - 10000;
SpscRing<BluetoothDevice, 8> BluetoothSystem::pendingDiscovered;
SpscRing<MacAddress, 8> BluetoothSystem::pendingDisconnected;
MacAddress BluetoothSystem::pendingDiscoveryMACs[7];
uint8_t BluetoothSystem::pendingDiscoveryCount = 0;
std::atomic<bool> BluetoothSystem::deviceListDirty{false};

void BluetoothSystem::init(IStorage* storage) {
    {
        BluetoothRegistryGuard guard;
        deviceList.clear();
    }
    _storage = storage;
    Bluefruit.begin(0, 10);
    Bluefruit.setName("OBike");

    // Increase Blink rate to different from PrPh advertising mode
    Bluefruit.setConnLedInterval(250);

    // Callbacks for Central
    Bluefruit.Central.setDisconnectCallback(BT_Device::disconnect_callback);
    Bluefruit.Central.setConnectCallback(connect_callback);
    BT_Device::onUnexpectedDisconnect(onDeviceUnexpectedDisconnect);

    /* Start Central Scanning
    * - Enable auto scan if disconnected
    * - Interval = 100 ms, window = 80 ms
    * - Don't use active scan
    * - Filter only accept csc service
    * - Start(timeout) with timeout = 0 will scan forever (until connected)
    */
    Bluefruit.Scanner.restartOnDisconnect(true);
    Bluefruit.Scanner.setInterval(160, 80);  // in unit of 0.625 ms
}

void BluetoothSystem::drainPendingEvents() {
    bool changed = false;
    bool disconnectedDevice = false;

    // Devices discovered by the scanner, queued from the BLE task.
    BluetoothDevice found;
    while (pendingDiscovered.pop(found)) {
        bool added = false;
        {
            BluetoothRegistryGuard guard;
            for (uint8_t i = 0; i < pendingDiscoveryCount; ++i) {
                if (pendingDiscoveryMACs[i] == found.MAC) {
                    for (uint8_t j = i + 1; j < pendingDiscoveryCount; ++j) {
                        pendingDiscoveryMACs[j - 1] = pendingDiscoveryMACs[j];
                    }
                    --pendingDiscoveryCount;
                    break;
                }
            }

            bool known = false;
            for (const auto& device : deviceList) {
                if (device.MAC == found.MAC) { known = true; break; }
            }
            if (!known) {
                deviceList.push_back(found);
                added = true;
            }
        }
        if (added) {
            Serial.print("New device found: "); Serial.println(found.name);
            changed = true;
        }
    }

    // Unexpected disconnects, queued from Bluefruit's disconnect callback.
    MacAddress gone;
    while (pendingDisconnected.pop(gone)) {
        // A device may have been removed after its callback queued this event;
        // only reconnect while it remains in the configured device registry.
        if (BT_Device::getDeviceWithMAC(gone)) disconnectedDevice = true;
        for (auto it = deviceList.begin(); it != deviceList.end(); it++) {
            if ((*it).MAC == gone) {
                (*it).connected = false;
                Serial.println("device disconnected");
                changed = true;
                break;
            }
        }
    }

    // Bluefruit's restartOnDisconnect only starts scanning when every central
    // connection has dropped. Resume here if a paired device disconnected
    // while another sensor is still connected.
    if (disconnectedDevice && _mode == E_Type_BT_Mode::connect &&
        !Bluefruit.Scanner.isRunning()) {
        if (!Bluefruit.Scanner.start(0)) {
            Serial.println("Couldn't restart Bluetooth scan after device disconnect");
        }
    }

    if (deviceListDirty.exchange(false)) {
        changed = true;
    }

    if (changed && deviceListCallback) {
        deviceListCallback(deviceList);
    }
}

void BluetoothSystem::update() {

    // Apply anything the BLE task queued since the last iteration, before
    // touching deviceList below.
    drainPendingEvents();

    switch (_mode) {
        case E_Type_BT_Mode::connect:
            if (_mode != _mode_prev) {
                Bluefruit.Scanner.stop();
                Bluefruit.Scanner.setRxCallback(scan_callback);
                Bluefruit.Scanner.filterUuid(GATT_CSC_UUID, UUID16_SVC_HEART_RATE, GATT_CPS_UUID, GATT_BAT_UUID);
                Bluefruit.Scanner.useActiveScan(true);
                Bluefruit.Scanner.start(0); // // 0 = Don't stop scanning after n seconds
            }
            break;

        case E_Type_BT_Mode::scan:
            if (_mode != _mode_prev) {
                Bluefruit.Scanner.stop();
                Bluefruit.Scanner.setRxCallback(scan_discovery);
                Bluefruit.Scanner.filterUuid(GATT_CSC_UUID, UUID16_SVC_HEART_RATE, GATT_CPS_UUID);
                Bluefruit.Scanner.useActiveScan(true);
                Bluefruit.Scanner.start(0);  // // 0 = Don't stop scanning after n seconds
            }
            break;

        case E_Type_BT_Mode::idle:
            if (_mode != _mode_prev) {
                Bluefruit.Scanner.stop();
            }
            break;

        default:
            if (_mode != _mode_prev) {
                Bluefruit.Scanner.stop();
            }
            break;
    }
    _mode_prev = _mode;
    uint32_t now = millis();
    for (auto it = deviceList.begin(); it != deviceList.end(); it++ ) {
        (*it).connected = BT_Device::deviceWithMacDiscovered((*it).MAC);
        if ((*it).connected) {
            const auto dev = BT_Device::getDeviceWithMAC((*it).MAC);
            if (!dev) continue;   // discovered but no longer in btDevices
            (*it).batt = dev->readBatt();
            dev->update(now);
        }
    }

    if (now - lastBatUpdate > 10000) {
        lastBatUpdate = now;
        if (deviceListCallback) {
            deviceListCallback(deviceList);
        }
    }
}
void BluetoothSystem::connect_callback(uint16_t conn_handle) {
    const auto device = BT_Device::getDeviceWithMAC(toConnectMAC);
    if (device) {
        if(!device->discovered()) {
            Serial.println("Not discovered");
            // discover() must run in this context -- it issues GATT reads --
            // but the UI notification must not. Flag it for update() instead.
            device->discover(conn_handle);
            deviceListDirty.store(true);
        }
    }
    if (!BT_Device::all_devices_discovered()) {
        Serial.println("More to discover");
        Bluefruit.Scanner.start(0);
    }
}
void BluetoothSystem::scan_callback(ble_gap_evt_adv_report_t* report) {
    Serial.println("Found Device:");
    Serial.print("MAC: ");
    Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
    Serial.print("\n");
    // Since we configure the scanner with filterUuid()
    // Scan callback only invoked for device with csc service advertised
    // Connect to device with csc service in advertising

    const auto device = BT_Device::getDeviceWithMAC(report->peer_addr.addr);
    if (device) {
        Serial.println("Match!");
        toConnectMAC = report->peer_addr.addr;
        Bluefruit.Central.connect(report);
    }else{
        Bluefruit.Scanner.resume();
    }
}

void BluetoothSystem::scan_discovery(ble_gap_evt_adv_report_t* report) {
    BluetoothDevice newDevice(report->peer_addr.addr);
    memset(newDevice.name, 0, sizeof(newDevice.name));

    // Leave room for a terminator: parseReportByType fills up to the length
    // given and does not NUL-terminate, so a name of exactly sizeof(name)
    // characters would leave the buffer unterminated for the Serial.println
    // and String conversion below.
    Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
                                        (uint8_t*)newDevice.name, sizeof(newDevice.name) - 1);

    if(Bluefruit.Scanner.checkReportForUuid(report, GATT_CSC_UUID))
        newDevice.type = E_Type_BT_Device::bt_csc;
    if(Bluefruit.Scanner.checkReportForUuid(report, UUID16_SVC_HEART_RATE))
        newDevice.type = E_Type_BT_Device::bt_hrm;
    if(Bluefruit.Scanner.checkReportForUuid(report, GATT_CPS_UUID))
        newDevice.type = E_Type_BT_Device::bt_cps;

    // Keep the duplicate check and enqueue atomic with respect to the main
    // loop draining the queue and appending to deviceList.
    {
        BluetoothRegistryGuard guard;
        bool known = false;
        for (const auto& device : deviceList) {
            if (device.MAC == newDevice.MAC) {
                known = true;
                break;
            }
        }
        for (uint8_t i = 0; !known && i < pendingDiscoveryCount; ++i) {
            known = pendingDiscoveryMACs[i] == newDevice.MAC;
        }

        if (!known &&
            pendingDiscoveryCount < (sizeof(pendingDiscoveryMACs) / sizeof(pendingDiscoveryMACs[0])) &&
            pendingDiscovered.push(newDevice)) {
            pendingDiscoveryMACs[pendingDiscoveryCount++] = newDevice.MAC;
        }
    }

    // For Softdevice v6: after received a report, scanner will be paused
    // We need to call Scanner resume() to continue scanning
    Bluefruit.Scanner.resume();
}

void BluetoothSystem::createDevice(const BluetoothDevice& device) {
    //check if the device has already been created
    const auto deviceptr = BT_Device::getDeviceWithMAC(device.MAC);

    //itterate the list of devices and set the save flage
    for (auto it = deviceList.begin(); it != deviceList.end(); it++ ) {
        if ((*it).MAC==device.MAC) {
            (*it).saved = true;
        }
    }

    //if not already in the list of bluetooth services create the appropriate one
    if (!deviceptr) {
        switch(device.type) {
            case E_Type_BT_Device::bt_csc:
                csc::create_csc(device.MAC);
                break;
            case E_Type_BT_Device::bt_hrm:
                hrm::create_hrm(device.MAC);
                break;
            case E_Type_BT_Device::bt_cps:
                cps::create_cps(device.MAC);
                break;
        }
    }
    if (deviceListCallback) {
        deviceListCallback(deviceList);
    }
}

void BluetoothSystem::disconnectDevice(const BluetoothDevice& device) {
    BT_Device::removeDeviceWithMAC(device.MAC);
    for (auto it = deviceList.begin(); it != deviceList.end(); it++ ) {
        if ( (*it).MAC == device.MAC) {
            (*it).connected = false;
            (*it).saved = false;
        }
    }
    if (deviceListCallback) {
        deviceListCallback(deviceList);
    }
}

void BluetoothSystem::onDeviceUnexpectedDisconnect(MacAddress MAC) {
    // Invoked from BT_Device::disconnect, which runs in Bluefruit's
    // disconnect callback -- i.e. on the BLE task. Queue it; update() marks
    // the device disconnected and notifies the UI.
    pendingDisconnected.push(MAC);
}

void BluetoothSystem::loadDevices() {
    if (_storage->exists("devices.txt")) {
        Serial.println("Found devices.txt");

        // Open file for reading
        File32 dataFile = _storage->openFile("/devices.txt", FILE_READ);
        // Allocate the memory pool on the stack.
        JsonDocument jsonBuffer;
        // Parse the root object

        DeserializationError error = deserializeJson(jsonBuffer, dataFile);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            return;
        }

        for (JsonObject device : jsonBuffer["devices"].as<JsonArray>()) {
            uint8_t MAC[6];

            JsonArray device_MAC = device["MAC"];
            MAC[0] = device_MAC[0];
            MAC[1] = device_MAC[1];
            MAC[2] = device_MAC[2];
            MAC[3] = device_MAC[3];
            MAC[4] = device_MAC[4];
            MAC[5] = device_MAC[5];

            BluetoothDevice newDevice(MAC);

            // Size argument must be the DESTINATION capacity, not the source
            // length -- newDevice.name is char[32] and this string comes from
            // a user-editable file on the SD card. toCharArray truncates and
            // NUL-terminates when given the real capacity.
            device["name"].as<String>().toCharArray(newDevice.name, sizeof(newDevice.name));

            newDevice.type = device["type"];
            newDevice.saved = true;

            createDevice(newDevice);
            {
                BluetoothRegistryGuard guard;
                deviceList.push_back(newDevice);
            }
            Serial.print("Loaded: "); Serial.println(newDevice.name);
        }
        dataFile.close();
    }
    if (deviceListCallback) {
        deviceListCallback(deviceList);
    }
}

void BluetoothSystem::saveDevices() {
    JsonDocument doc;
    JsonArray devices = doc["devices"].to<JsonArray>();
    uint16_t j = 0;

    for (uint16_t i = 0; i < deviceList.size(); i++) {
        if (deviceList[i].saved) {
            devices[j]["name"] = deviceList[i].name;
            devices[j]["type"] = deviceList[i].type;
            JsonArray device_MAC = devices[j]["MAC"].to<JsonArray>();
            ;
            device_MAC.add(deviceList[i].MAC[0]);
            device_MAC.add(deviceList[i].MAC[1]);
            device_MAC.add(deviceList[i].MAC[2]);
            device_MAC.add(deviceList[i].MAC[3]);
            device_MAC.add(deviceList[i].MAC[4]);
            device_MAC.add(deviceList[i].MAC[5]);
            j++;
        }
    }

    if (_storage->exists("/devices.txt"))
        _storage->remove("/devices.txt");

    File32 dataFile = _storage->openFile("/devices.txt", FILE_WRITE);

    serializeJson(doc, dataFile);
    dataFile.close();
}
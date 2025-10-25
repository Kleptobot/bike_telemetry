#include "BluetoothSystem.hpp"

MacAddress BluetoothSystem::toConnectMAC;
std::vector<BluetoothDevice> BluetoothSystem::deviceList;
BluetoothSystem::DeviceListCallback BluetoothSystem::deviceListCallback;
E_Type_BT_Mode BluetoothSystem::_mode, BluetoothSystem::_mode_prev;

void BluetoothSystem::init() {
    deviceList.clear();
    Bluefruit.begin(0, 10);
    Bluefruit.setName("OBike");

    // Increase Blink rate to different from PrPh advertising mode
    Bluefruit.setConnLedInterval(250);

    // Callbacks for Central
    Bluefruit.Central.setDisconnectCallback(BT_Device::disconnect_callback);
    Bluefruit.Central.setConnectCallback(connect_callback);

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

void BluetoothSystem::update() {
    
    switch (_mode) {
        case E_Type_BT_Mode::connect:
            if (_mode != _mode_prev) {
                Bluefruit.Scanner.setRxCallback(scan_callback);
                Bluefruit.Scanner.filterUuid(GATT_CSC_UUID, UUID16_SVC_HEART_RATE, GATT_CPS_UUID, GATT_BAT_UUID);
                Bluefruit.Scanner.useActiveScan(true);
                Bluefruit.Scanner.start(0); // // 0 = Don't stop scanning after n seconds
            }
            break;

        case E_Type_BT_Mode::scan:
            if (_mode != _mode_prev) {
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
    for (auto it = deviceList.begin(); it != deviceList.end(); it++ ) {
        (*it).connected = BT_Device::deviceWithMacDiscovered((*it).MAC);
    }
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void BluetoothSystem::connect_callback(uint16_t conn_handle) {
    BT_Device* device = BT_Device::getDeviceWithMAC(toConnectMAC);
    if (device != NULL) {
        if(!device->discovered()) {
            device->discover(conn_handle);
            if (deviceListCallback) {
                deviceListCallback(deviceList);
            }
        }
    }
    if (!BT_Device::all_devices_discovered()) {
        Bluefruit.Scanner.resume();
    }
}

/**
 * Callback invoked when scanner pick up an advertising data
 * @param report Structural advertising data
 */
void BluetoothSystem::scan_callback(ble_gap_evt_adv_report_t* report) {
    Serial.println("Found Device:");
    Serial.print("MAC: ");
    Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
    Serial.print("\n");
    // Since we configure the scanner with filterUuid()
    // Scan callback only invoked for device with csc service advertised
    // Connect to device with csc service in advertising

    BT_Device* device = BT_Device::getDeviceWithMAC(report->peer_addr.addr);
    if (device != NULL) {
        Serial.println("Match!");
        toConnectMAC = report->peer_addr.addr;
        Bluefruit.Central.connect(report);
    }
}

/**
 * Callback invoked when scanner pick up an advertising data
 * @param report Structural advertising data
 */
void BluetoothSystem::scan_discovery(ble_gap_evt_adv_report_t* report) {
    BluetoothDevice newDevice(report->peer_addr.addr);
    memset(&newDevice.name,0,32);

    Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, (uint8_t*)newDevice.name, sizeof(newDevice.name));

    bool bMatch = 0;
    //check if the list contains anyhting
    if (deviceList.size() > 0) {
        //iterate the list
        for (uint16_t i = 0; i < deviceList.size(); i++) {
            //check if the new found device matches any of the devices in the list
            if (deviceList[i].MAC==newDevice.MAC) {
            bMatch = 1;
            }
        }
    }
    //if the new device is unique add it
    if (!bMatch) {
    if(Bluefruit.Scanner.checkReportForUuid(report, GATT_CSC_UUID))
        newDevice.type = E_Type_BT_Device::bt_csc;
    if(Bluefruit.Scanner.checkReportForUuid(report, UUID16_SVC_HEART_RATE))
        newDevice.type = E_Type_BT_Device::bt_hrm;
    if(Bluefruit.Scanner.checkReportForUuid(report, GATT_CPS_UUID))
        newDevice.type = E_Type_BT_Device::bt_cps;

    deviceList.push_back(newDevice);
    }

    // For Softdevice v6: after received a report, scanner will be paused
    // We need to call Scanner resume() to continue scanning
    Bluefruit.Scanner.resume();
}

void BluetoothSystem::createDevice(const BluetoothDevice& device) {
    BT_Device* deviceptr = BT_Device::getDeviceWithMAC(device.MAC);
    if ( !deviceptr) {
        switch(device.type) {
            case E_Type_BT_Device::bt_csc:
                csc::create_csc(device.name, 32, device.MAC);
                break;
            case E_Type_BT_Device::bt_hrm:
                hrm::create_hrm(device.name, 32, device.MAC);
                break;
            case E_Type_BT_Device::bt_cps:
                cps::create_cps(device.name, 32, device.MAC);
                break;
        }
    }
}

void BluetoothSystem::disconnectDevice(const BluetoothDevice& device) {
    BT_Device::removeDeviceWithMAC(device.MAC);
    for (auto it = deviceList.begin(); it != deviceList.end(); it++ ) {
        (*it).connected = BT_Device::deviceWithMacDiscovered((*it).MAC);
    }
    if (deviceListCallback) {
        deviceListCallback(deviceList);
    }
}

void BluetoothSystem::loadDevices(SdFat32* SD) {
    if (SD->exists("devices.txt")) {
        // Open file for reading
        File32 dataFile = SD->open("/devices.txt", FILE_READ);
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

            device["name"].as<String>().toCharArray(newDevice.name, device["name"].as<String>().length() + 1);
            
            newDevice.type = device["type"];

            createDevice(newDevice);
            deviceList.push_back(newDevice);
        }
        dataFile.close();
    }
}

void BluetoothSystem::saveDevices(SdFat32* SD) {
    JsonDocument doc;
    JsonArray devices = doc["devices"].to<JsonArray>();
    uint16_t j = 0;

    for (uint16_t i = 0; i < deviceList.size(); i++) {
        if (deviceList[i].connected) {
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

    if (SD->exists("/devices.txt"))
        SD->remove("/devices.txt");

    File32 dataFile = SD->open("/devices.txt", FILE_WRITE);

    serializeJson(doc, dataFile);
    dataFile.close();
}
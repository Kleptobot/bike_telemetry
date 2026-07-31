// Host Bluefruit stub.
//
// Deliberately more than a compile-satisfier. The real csc.cpp, cps.cpp and
// hrm.cpp are compiled into the simulator, so their measurement parsers -- the
// code most likely to be wrong, and the code the CPS rewrite touched -- run
// exactly as they do on device. This stub provides the plumbing needed to
// drive them: BLEClientCharacteristic remembers its notify callback, and
// simNotify() invokes it with a caller-supplied payload.
//
// That means a scenario can push a spec-shaped Cycling Power Measurement into
// the firmware and watch the power/cadence tiles respond, without a radio.
#ifndef BLUEFRUIT_H_STUB
#define BLUEFRUIT_H_STUB

#include "Arduino.h"
#include <vector>
#include <cstring>

#define UUID16_SVC_HEART_RATE               0x180D
#define UUID16_CHR_HEART_RATE_MEASUREMENT   0x2A37
#define UUID16_CHR_BODY_SENSOR_LOCATION     0x2A38
#define BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME 0x09
#define BLE_CONN_HANDLE_INVALID             0xFFFF

typedef struct {
    uint8_t addr[6];
} sim_gap_addr_t;

typedef struct {
    sim_gap_addr_t peer_addr;
    // Advertised 16-bit service UUIDs, used by checkReportForUuid.
    uint16_t uuids[4];
    uint8_t  uuidCount;
    char     name[32];
} ble_gap_evt_adv_report_t;

class BLEClientCharacteristic;
typedef void (*notify_cb_t)(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

class BLEClientService {
public:
    explicit BLEClientService(uint16_t uuid) : _uuid(uuid) {}
    bool begin() { return true; }
    bool discover(uint16_t conn_handle) { _conn = conn_handle; return _discoverable; }
    uint16_t connHandle() const { return _conn; }
    uint16_t uuid() const { return _uuid; }

    // Simulator hook: decides whether a service "exists" on the peer.
    void simSetDiscoverable(bool v) { _discoverable = v; }

private:
    uint16_t _uuid;
    uint16_t _conn = BLE_CONN_HANDLE_INVALID;
    bool _discoverable = true;
};

class BLEClientCharacteristic {
public:
    explicit BLEClientCharacteristic(uint16_t uuid) : _uuid(uuid) { all().push_back(this); }

    bool begin() { return true; }
    bool discover() { _discovered = true; return true; }
    bool discovered() const { return _discovered; }
    bool enableNotify() { _notifyEnabled = true; return true; }
    void setNotifyCallback(notify_cb_t cb) { _cb = cb; }
    uint16_t connHandle() const { return _conn; }
    uint16_t uuid() const { return _uuid; }

    uint8_t  read8()  { return _r8; }
    uint16_t read16() { return _r16; }

    // --- simulator hooks -------------------------------------------------
    void simSetConnHandle(uint16_t h) { _conn = h; }
    void simSetRead8(uint8_t v)  { _r8 = v; }
    void simSetRead16(uint16_t v) { _r16 = v; }

    /** Invokes the firmware's notify callback with a raw payload. */
    void simNotify(const uint8_t* data, uint16_t len) {
        if (_cb) _cb(this, const_cast<uint8_t*>(data), len);
    }

    /** Every characteristic constructed, so a scenario can find one by UUID. */
    static std::vector<BLEClientCharacteristic*>& all() {
        static std::vector<BLEClientCharacteristic*> v;
        return v;
    }
    static BLEClientCharacteristic* simFind(uint16_t uuid, uint16_t connHandle) {
        for (auto* c : all()) if (c->uuid() == uuid && c->connHandle() == connHandle) return c;
        return nullptr;
    }

private:
    uint16_t _uuid;
    uint16_t _conn = BLE_CONN_HANDLE_INVALID;
    bool _discovered = false;
    bool _notifyEnabled = false;
    notify_cb_t _cb = nullptr;
    uint8_t  _r8 = 0;
    uint16_t _r16 = 0;
};

typedef void (*connect_cb_t)(uint16_t conn_handle);
typedef void (*disconnect_cb_t)(uint16_t conn_handle, uint8_t reason);
typedef void (*scan_cb_t)(ble_gap_evt_adv_report_t* report);

class BluefruitCentral {
public:
    void setConnectCallback(connect_cb_t cb) { onConnect = cb; }
    void setDisconnectCallback(disconnect_cb_t cb) { onDisconnect = cb; }
    void connect(ble_gap_evt_adv_report_t* report);

    connect_cb_t onConnect = nullptr;
    disconnect_cb_t onDisconnect = nullptr;
};

class BluefruitScanner {
public:
    void start(uint16_t = 0) { _running = true; }
    void stop() { _running = false; }
    void resume() { _running = true; }
    void restartOnDisconnect(bool) {}
    void setInterval(uint16_t, uint16_t) {}
    void useActiveScan(bool) {}
    void setRxCallback(scan_cb_t cb) { _rx = cb; }
    void filterUuid(uint16_t a = 0, uint16_t b = 0, uint16_t c = 0, uint16_t d = 0) {
        _filter[0] = a; _filter[1] = b; _filter[2] = c; _filter[3] = d;
    }
    bool checkReportForUuid(ble_gap_evt_adv_report_t* r, uint16_t uuid) {
        for (uint8_t i = 0; i < r->uuidCount; i++) if (r->uuids[i] == uuid) return true;
        return false;
    }
    uint8_t parseReportByType(ble_gap_evt_adv_report_t* r, uint8_t type, uint8_t* buf, uint8_t len) {
        if (type != BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME || !buf || !len) return 0;
        uint8_t n = (uint8_t)strnlen(r->name, sizeof(r->name));
        if (n > len) n = len;
        memcpy(buf, r->name, n);
        return n;
    }

    /** Delivers a synthetic advertisement to the firmware's scan callback. */
    void simAdvertise(ble_gap_evt_adv_report_t* report) {
        if (_running && _rx) _rx(report);
    }
    bool running() const { return _running; }

private:
    scan_cb_t _rx = nullptr;
    bool _running = false;
    uint16_t _filter[4] = {0, 0, 0, 0};
};

class BluefruitStub {
public:
    bool begin(uint8_t = 0, uint8_t = 0) { return true; }
    void setName(const char*) {}
    void setConnLedInterval(uint32_t) {}
    void disconnect(uint16_t conn_handle);

    BluefruitCentral Central;
    BluefruitScanner Scanner;
};

extern BluefruitStub Bluefruit;

#endif /* BLUEFRUIT_H_STUB */

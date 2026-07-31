#include "bluefruit.h"
#include "SPI.h"
#include "Wire.h"

BluefruitStub Bluefruit;
SPIClass SPI;
TwoWire Wire;

// A synthetic connection handle allocator. Each simulated peripheral gets a
// distinct handle so the firmware's per-connection dispatch (which matches
// chr->connHandle() against a device's service handle) works unmodified.
static uint16_t g_nextHandle = 1;

void BluefruitCentral::connect(ble_gap_evt_adv_report_t* report) {
    (void)report;
    const uint16_t handle = g_nextHandle++;
    if (onConnect) onConnect(handle);
}

void BluefruitStub::disconnect(uint16_t conn_handle) {
    if (Central.onDisconnect) Central.onDisconnect(conn_handle, 0x13 /* remote user terminated */);
}

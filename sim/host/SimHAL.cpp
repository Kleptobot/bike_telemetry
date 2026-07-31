// Link-time replacement for src/HAL/HAL.cpp.
//
// This is the ONLY firmware translation unit the simulator substitutes.
// Everything else -- InputSystem, Sensors, SDCard, AltitudeFusion, button,
// and the whole Bluetooth stack including the CSC/CPS/HRM measurement parsers
// -- is compiled from the real source and runs unmodified. So does all of
// App, DataModel, Loggers, Map and the UI.
//
// Because these are HAL member functions, they can reach the private _LC76G
// member. That lets the simulator feed synthetic NMEA through the real
// TinyGPSPlus parser rather than faking the getters, so the GPS path
// (validity transitions, speed, altitude age) exercises real code.

#include "HAL/HAL.hpp"
#include "SimHAL.hpp"
#include "Adafruit_MCP23X17.h"
#include <cstdio>
#include <cmath>

namespace Sim {

static State g_state;
State& state() { return g_state; }

static bool g_buttons[6] = {false, false, false, false, false, false};

void setButton(ButtonName b, bool pressed) { g_buttons[(int)b] = pressed; }
bool buttonState(ButtonName b) { return g_buttons[(int)b]; }
void pressButton(ButtonName b)   { setButton(b, true); }
void releaseButton(ButtonName b) { setButton(b, false); }

// --- NMEA generation ------------------------------------------------------
// Real sentences with real checksums, so TinyGPSPlus does the parsing.

static uint8_t nmeaChecksum(const char* body) {
    uint8_t c = 0;
    for (const char* p = body; *p; ++p) c ^= (uint8_t)*p;
    return c;
}

static void feed(TinyGPSPlus& gps, const char* body) {
    char line[128];
    snprintf(line, sizeof(line), "$%s*%02X\r\n", body, nmeaChecksum(body));
    for (const char* p = line; *p; ++p) gps.encode(*p);
}

/** Emits one RMC + GGA pair describing the current simulated position. */
static void feedNmea(TinyGPSPlus& gps, uint32_t simSeconds) {
    const State& s = g_state;

    const double absLat = fabs(s.latitude);
    const double absLon = fabs(s.longitude);
    const int latDeg = (int)absLat;
    const int lonDeg = (int)absLon;
    const double latMin = (absLat - latDeg) * 60.0;
    const double lonMin = (absLon - lonDeg) * 60.0;
    const char latHem = s.latitude >= 0 ? 'N' : 'S';
    const char lonHem = s.longitude >= 0 ? 'E' : 'W';

    const uint32_t hh = (simSeconds / 3600) % 24;
    const uint32_t mm = (simSeconds / 60) % 60;
    const uint32_t ss = simSeconds % 60;

    char body[128];

    // RMC carries time, validity, speed and date.
    snprintf(body, sizeof(body),
             "GPRMC,%02u%02u%02u.00,%c,%02d%09.6f,%c,%03d%09.6f,%c,%.2f,0.00,010126,,,%c",
             hh, mm, ss, s.gpsValid ? 'A' : 'V',
             latDeg, latMin, latHem, lonDeg, lonMin, lonHem,
             s.gpsSpeedKmh / 1.852f,           // NMEA speed is knots
             s.gpsValid ? 'A' : 'N');
    feed(gps, body);

    // GGA carries fix quality, satellite count and altitude.
    snprintf(body, sizeof(body),
             "GPGGA,%02u%02u%02u.00,%02d%09.6f,%c,%03d%09.6f,%c,%d,09,0.9,%.1f,M,0.0,M,,",
             hh, mm, ss,
             latDeg, latMin, latHem, lonDeg, lonMin, lonHem,
             s.gpsValid ? 1 : 0, s.altitude);
    feed(gps, body);
}

// --- BLE injection --------------------------------------------------------

uint16_t buildCscMeasurement(uint8_t* out, uint32_t wheelRevs, uint16_t wheelEventTime,
                             uint16_t crankRevs, uint16_t crankEventTime) {
    // CSC Measurement: uint8 flags, then optional wheel (u32+u16) and crank
    // (u16+u16) blocks. Flags here set both present.
    uint16_t i = 0;
    out[i++] = 0x03;
    memcpy(out + i, &wheelRevs, 4); i += 4;
    memcpy(out + i, &wheelEventTime, 2); i += 2;
    memcpy(out + i, &crankRevs, 2); i += 2;
    memcpy(out + i, &crankEventTime, 2); i += 2;
    return i;
}

uint16_t buildCpsMeasurement(uint8_t* out, int16_t watts,
                             uint16_t crankRevs, uint16_t crankEventTime) {
    // Cycling Power Measurement: uint16 flags, sint16 power, then optional
    // fields. Bit 5 = crank revolution data present.
    uint16_t i = 0;
    const uint16_t flags = 0x0020;
    memcpy(out + i, &flags, 2); i += 2;
    memcpy(out + i, &watts, 2); i += 2;
    memcpy(out + i, &crankRevs, 2); i += 2;
    memcpy(out + i, &crankEventTime, 2); i += 2;
    return i;
}

uint16_t buildHrmMeasurement(uint8_t* out, uint16_t bpm) {
    // Heart Rate Measurement: uint8 flags (bit0 clear = uint8 value).
    out[0] = 0x00;
    out[1] = (uint8_t)bpm;
    return 2;
}

static void injectByUuid(uint16_t uuid, const uint8_t* data, uint16_t len) {
    for (auto* chr : BLEClientCharacteristic::all()) {
        if (chr->uuid() == uuid) { chr->simNotify(data, len); return; }
    }
}

void injectCscNotification(const uint8_t* d, uint16_t n) { injectByUuid(0x2A5B, d, n); }
void injectCpsNotification(const uint8_t* d, uint16_t n) { injectByUuid(0x2A63, d, n); }
void injectHrmNotification(const uint8_t* d, uint16_t n) { injectByUuid(0x2A37, d, n); }

/** Composes the MCP port-A byte the real InputSystem will read. */
static void publishButtons() {
    uint8_t a = 0;
    if (g_buttons[(int)ButtonName::Up])     a |= (1 << SIM_MCP_BIT_UP);
    if (g_buttons[(int)ButtonName::Down])   a |= (1 << SIM_MCP_BIT_DOWN);
    if (g_buttons[(int)ButtonName::Left])   a |= (1 << SIM_MCP_BIT_LEFT);
    if (g_buttons[(int)ButtonName::Right])  a |= (1 << SIM_MCP_BIT_RIGHT);
    if (g_buttons[(int)ButtonName::Select]) a |= (1 << SIM_MCP_BIT_SELECT);
    // SD_DET is inverted: the bit is SET when no card is present.
    if (!g_state.sdPresent) a |= (1 << SIM_MCP_BIT_SD_DET);
    simSetMcpGPIOA(a);
}

void tick(uint32_t) { publishButtons(); }

} // namespace Sim

// ---------------------------------------------------------------------------
// HAL member definitions
// ---------------------------------------------------------------------------

void HAL::init_low() {
    inputSystem.init();
    sensorSystem.init_low();
    _resetGPSTime = 0;
    _sleep = false;
}

void HAL::init(timeData* date) {
    Wire.begin();
    sensorSystem.init();
    bluetoothSystem.init(&storageSystem);
    storageSystem.init(date);
    Serial.println("[sim] HAL initialised");
}

void HAL::update() {
    // Feed the real NMEA parser so GPS validity, speed and altitude all come
    // through TinyGPSPlus exactly as they do on device.
    static uint32_t lastNmea = 0;
    if (millis() - lastNmea >= 1000) {
        lastNmea = millis();
        Sim::feedNmea(_LC76G.gps(), millis() / 1000);
    }

    Sim::tick(0);
    inputSystem.update(false);
    sensorSystem.update(false);
    bluetoothSystem.update();

    const Sim::State& s = Sim::state();

    // Altitude comes from the simulated barometer via AltitudeFusion, unless
    // the scenario is driving it directly.
    f32_alt   = s.altitude;
    _dpsValid = true;

    wheelRPM = {s.wheelRPM, s.wheelRPMLive};
    gpsKmh   = {s.gpsSpeedKmh, s.gpsValid};

    // Prefer live BLE values (injected notifications drive the real parsers);
    // fall back to whatever the scenario set directly.
    f32_cadence = csc::getCadence().live ? csc::getCadence().value : s.cadence;
    f32_bpm     = hrm::getHRM().live     ? hrm::getHRM().value     : s.heartRate;
    f32_pow     = cps::getPower().live   ? cps::getPower().value   : s.power;
    f32_temp    = s.temperature;

    if (_resetGPSTime > 0 && millis() - _resetGPSTime > 100) _resetGPSTime = 0;
    if (_resetDispTime > 0 && millis() - _resetDispTime > 100) _resetDispTime = 0;
}

void HAL::resetGPS()     { _resetGPSTime = millis(); }
void HAL::resetDisplay() { _resetDispTime = millis(); }
void HAL::buzzStart()    {}
void HAL::buzzStop()     {}

void HAL::sleep() {
    Serial.println("[sim] sleep requested");
    _sleep = true;
}

void HAL::setNMEArates(uint8_t type, uint8_t rate) {
    (void)type; (void)rate;   // no radio to configure
}

void HAL::onSleep(int, const void*, void*) {}
void HAL::onPAIRResponse(int, const void*, void*) {}
void HAL::handlePAIRResponse(int, const void*) {}

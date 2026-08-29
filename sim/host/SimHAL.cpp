// Link-time replacement for src/HAL/HAL.cpp.
//
// This is the ONLY firmware translation unit the simulator substitutes.
// Everything else -- InputSystem, Sensors, SDCard, the whole Fusion layer
// (altitude fusion, speed/grade/distance estimation), button, and the entire
// Bluetooth stack including the CSC/CPS/HRM measurement parsers -- is
// compiled from the real source and runs unmodified. So does all of App,
// DataModel, Loggers, Map and the UI.
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
#include <fstream>

// Scenario-driven barometer hooks (defined in stubs/Dps3xx.cpp).
extern void simSetPressurePa(float pa);
extern void simSetBaroTempC(float c);

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
// Simulated BLE sensors
//
// devices.txt on the simulated SD creates the parser devices through the real
// loadDevices() path at App BOOT. Once they exist we complete the radio
// story: each device runs its real discover() against the stub radio, and the
// stub characteristics' connection handles are aligned with their service
// handles -- which is what an established connection provides for
// notification dispatch on real hardware.
// ---------------------------------------------------------------------------

// MACs must match the devices.txt fixture written by ensureSensorFixture()
// (type 0=csc, 1=hrm, 2=cps).
static const uint8_t kWheelMac[6] = {2, 0, 0, 0, 0, 1};
static const uint8_t kPowerMac[6] = {2, 0, 0, 0, 0, 2};
static const uint8_t kHeartMac[6] = {2, 0, 0, 0, 0, 3};

// Seeds the simulated SD with the paired-sensor fixture if the card does not
// already carry one, so the real loadDevices() path at App BOOT has the same
// three devices the connection step expects. Also seeds a 2x2 main-screen
// tile layout (Speed / Power / Cadence / HeartRate) so headless runs show
// live data instead of an empty grid. sim/sdcard is gitignored -- the card is
// regenerated, not vendored -- and any file that is already there (e.g. the
// user's real paired sensors or their own layout) is left untouched.
static void ensureSensorFixture() {
    const std::string dev = simSdHostPath("devices.txt");
    std::ifstream existing(dev.c_str());
    if (!existing.good()) {
        existing.close();
        std::ofstream out(dev.c_str());
        out << "{\"devices\":["
            << "{\"name\":\"Sim Wheel\",\"type\":0,\"MAC\":[2,0,0,0,0,1]},"
            << "{\"name\":\"Sim Power\",\"type\":2,\"MAC\":[2,0,0,0,0,2]},"
            << "{\"name\":\"Sim Heart\",\"type\":1,\"MAC\":[2,0,0,0,0,3]}"
            << "]}\n";
    }
    const std::string lay = simSdHostPath("layout.txt");
    std::ifstream layIn(lay.c_str());
    if (!layIn.good()) {
        layIn.close();
        std::ofstream out(lay.c_str());
        // Same shape App::saveLayout() writes and App::loadLayout() parses.
        out << "{\"displays\":["
            << "{\"x0\":0,\"y0\":0,\"x1\":1,\"y1\":1,\"type\":\"Speed\"},"
            << "{\"x0\":1,\"y0\":0,\"x1\":2,\"y1\":1,\"type\":\"Power\"},"
            << "{\"x0\":0,\"y0\":1,\"x1\":1,\"y1\":2,\"type\":\"Cadence\"},"
            << "{\"x0\":1,\"y0\":1,\"x1\":2,\"y1\":2,\"type\":\"HeartRate\"}"
            << "],\"rows\":2,\"cols\":2}\n";
    }
}

// Runs the device's real discovery path against the stub radio, then aligns
// the stub-side handles. The feature characteristic gets a sane value: on a
// real sensor the CSC feature byte advertises wheel+crank support, and
// csc::discover() uses it to set b_speed_present/b_cadence_present -- with a
// stub default of 0 the decoders would stay dormant no matter what we inject.
// The reads must be programmed BEFORE discover() runs: discover consumes them
// exactly as a real GATT read would.
static bool connectSensor(const uint8_t* mac, uint16_t conn,
                          uint16_t measUuid, uint16_t featUuid,
                          uint8_t feat8, uint16_t feat16) {
    BT_Device* dev = BT_Device::getDeviceWithMAC(MacAddress(mac));
    if (!dev) return false;   // loadDevices() has not run yet (App BOOT)
    for (auto* chr : BLEClientCharacteristic::all()) {
        if (chr->uuid() == featUuid) {
            chr->simSetRead8(feat8);
            chr->simSetRead16(feat16);
        }
        if (chr->uuid() == 0x2A19) chr->simSetRead8(100);   // battery: full
    }
    dev->discover(conn);
    // Align the stub characteristic handles with their services, matching
    // the state an established connection provides for notification dispatch.
    for (auto* chr : BLEClientCharacteristic::all()) {
        if (chr->uuid() == measUuid || chr->uuid() == featUuid)
            chr->simSetConnHandle(conn);
    }
    return true;
}

static bool connectSimSensors() {
    bool ok = true;
    ok &= connectSensor(kWheelMac, 1, 0x2A5B, 0x2A5C, 0x03, 0);
    ok &= connectSensor(kPowerMac, 2, 0x2A63, 0x2A65, 0, 0x0020);
    ok &= connectSensor(kHeartMac, 3, 0x2A37, 0,      0, 0);
    return ok;
}

// Builds spec-shaped measurement payloads from the scenario and hands them to
// the real parsers at the ~1 Hz notification cadence real sensors use.
// Wheel/crank revolutions accumulate from the requested rates; the parsers'
// first-packet baselining absorbs the startup, after which the decoded RPM is
// what the scenario asked for (modulo the parsers' own smoothing filters).
static void injectSimSensors() {
    const Sim::State& st = Sim::state();
    static float wheelRevs = 0.0f, crankRevs = 0.0f, evtSec = 0.0f;
    const float dtS = 1.0f;   // injection cadence, seconds
    evtSec    += dtS;
    wheelRevs += st.wheelRPM * dtS / 60.0f;
    crankRevs += st.cadence  * dtS / 60.0f;
    const uint16_t evt1024 = (uint16_t)(evtSec * 1024.0f);   // 1/1024 s clock

    uint8_t buf[20];
    if (st.wheelRPMLive) {
        const uint16_t n = Sim::buildCscMeasurement(buf, (uint32_t)wheelRevs,
                                                    evt1024,
                                                    (uint16_t)crankRevs, evt1024);
        Sim::injectCscNotification(buf, n);
    }
    if (st.power > 0.0f) {
        const uint16_t n = Sim::buildCpsMeasurement(buf, (int16_t)(st.power + 0.5f),
                                                    (uint16_t)crankRevs, evt1024);
        Sim::injectCpsNotification(buf, n);
    }
    if (st.heartRate > 0.0f) {
        const uint16_t n = Sim::buildHrmMeasurement(buf, (uint16_t)(st.heartRate + 0.5f));
        Sim::injectHrmNotification(buf, n);
    }
}

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
    // Mirror the real HAL::init's GPIO setup. GPIOB3 in particular is the GPS
    // enable line, and InputSystem reads it back to decide whether the device
    // should sleep -- main.cpp's loop() does
    //     if (!getGpsEnableState()) { delay(200); NRF_POWER->SYSTEMOFF = 1; }
    // so leaving it low made every simulated loop burn 200 ms of simulated
    // time, which made held-button repeat fire ~40x too fast.
    inputSystem.setOutput(GPIOB3, true);
    inputSystem.update(false);

    Wire.begin();
    sensorSystem.init();
    bluetoothSystem.init(&storageSystem);
    storageSystem.init(date);
    ensureSensorFixture();

    inputSystem.setOutput(GPIOB6, true);   // backlight, as the real init does
    inputSystem.update(false);

    Serial.println("[sim] HAL initialised");
}

void HAL::update() {
    _tickStartMs = millis();

    // Feed the real NMEA parser so GPS validity, speed and altitude all come
    // through TinyGPSPlus exactly as they do on device.
    static uint32_t lastNmea = 0;
    if (millis() - lastNmea >= 1000) {
        lastNmea = millis();
        Sim::feedNmea(_LC76G.gps(), millis() / 1000);
    }

    // --- Simulated BLE sensors -------------------------------------------------
    // loadDevices() (App BOOT) must have created the parser devices before the
    // discovery step can run, hence the lazy connect. Injection then flows at
    // 1 Hz for as long as the scenario says a sensor is present.
    static bool sensorsConnected = false;
    if (!sensorsConnected && millis() > 1500) {
        sensorsConnected = connectSimSensors();
    }

    static uint32_t lastBleInject = 0;
    if (sensorsConnected && millis() - lastBleInject >= 1000) {
        lastBleInject = millis();
        injectSimSensors();
    }

    const Sim::State& s = Sim::state();

    // Drive the simulated barometer from the scenario altitude so the real
    // fusion path (baro -> FusionEngine -> altitude/vario/grade) runs in the
    // simulator exactly as it does on device. Pressure is derived from
    // altitude with the standard-atmosphere lapse relative to 101325 Pa; the
    // estimator's own P0 calibration absorbs any absolute offset, and
    // pressure deltas across ticks give the vario a genuine climb signal.
    simSetPressurePa(101325.0f * powf(1.0f - (0.0065f * s.altitude)
                                                  / (s.temperature + 273.15f),
                                      5.2558797f));
    simSetBaroTempC(s.temperature);

    Sim::tick(0);
    inputSystem.update(false);
    sensorSystem.update(false);
    bluetoothSystem.update();

    if (_resetGPSTime > 0 && millis() - _resetGPSTime > 100) _resetGPSTime = 0;
    if (_resetDispTime > 0 && millis() - _resetDispTime > 100) _resetDispTime = 0;

    // Assemble the measurement frame for this tick (see HAL/Measurements.hpp)
    refreshFrame();
}

void HAL::resetGPS()     { _resetGPSTime = millis(); }
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

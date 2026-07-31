// Link-time replacement for src/HAL/LC76G.cpp.
//
// The LC76G driver is an I2C transport plus an NMEA assembler. Neither is
// meaningful without a radio, and the simulator feeds NMEA straight into the
// TinyGPSPlus instance from SimHAL, so the transport is stubbed out here.
//
// The methods that remain are the ones HAL.hpp calls inline (sendCommand from
// gpsHotStart/gpsSaveNVRAM/setRMCRate and friends) plus the lifecycle hooks.
// They log rather than doing nothing silently, so a scenario that exercises
// the GPS settings screen still shows which commands the firmware issued.
#include "HAL/LC76G.hpp"

bool LC76G::begin(TwoWire* wire) { _wire = wire; _state = STATE_IDLE; return true; }

void LC76G::update() { /* NMEA is injected directly by SimHAL */ }

LC76G::State LC76G::stateMachine() { return _state; }

void LC76G::sendCommand(CmdId cmdId, ResponseCallback cb, void* userCtx, const void* payload) {
    (void)payload;
    const uint16_t index = getCommand(cmdId);
    Serial.print("[sim] LC76G command: ");
    Serial.println(index < std::size(COMMANDS) ? COMMANDS[index].cmd : "<unknown>");

    // Acknowledge immediately so response-timeout paths do not accumulate.
    if (cb) cb(0, nullptr, userCtx);
}

uint16_t LC76G::getReceivedData(uint8_t*, uint16_t) { return 0; }
bool LC76G::queueCommand(const uint8_t*, uint16_t) { return true; }
bool LC76G::i2cWrite(uint8_t, const uint8_t*, uint16_t) { return true; }
bool LC76G::i2cRead(uint8_t, uint8_t*, uint16_t) { return false; }
bool LC76G::sendReadRequest(uint16_t, uint16_t) { return true; }
bool LC76G::sendWriteRequest(uint16_t, uint16_t) { return true; }
bool LC76G::readResponse(uint8_t*, uint16_t) { return false; }
bool LC76G::writeData(const uint8_t*, uint16_t) { return true; }
int  LC76G::Recovery_I2c() { return 0; }
uint8_t LC76G::calculate_xor_checksum(const uint8_t* d, size_t n) {
    uint8_t c = 0; for (size_t i = 0; i < n; ++i) c ^= d[i]; return c;
}
void LC76G::addSentence() {}
void LC76G::pollResponseTimeouts() {}
void LC76G::processSentence(Sentence) {}

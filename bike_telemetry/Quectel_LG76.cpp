#include "Quectel_LG76.h"

Quectel_LG76::Quectel_LG76(TwoWire &w): wire(w), state(S_IDLE), busLocked(false),
    tReady(0), availLen(0), rxLen(0), rxBuf(nullptr), txCmd(nullptr), txLen(0) {}

void Quectel_LG76::begin() { wire.begin(); }

void Quectel_LG76::putLE32(uint8_t *dst, uint32_t v) {
  dst[0] = v & 0xFF; dst[1] = v >> 8; dst[2] = v >> 16; dst[3] = v >> 24;
}

uint8_t Quectel_LG76::calcChecksum(const char *cmd) {
  uint8_t chk = 0; if (*cmd=='$') cmd++;
  while (*cmd && *cmd!='*') chk ^= (uint8_t)(*cmd++);
  return chk;
}

void Quectel_LG76::makePMTK(const char *cmd, char *out) {
  uint8_t c = calcChecksum(cmd);
  sprintf(out, "%s*%02X\r\n", cmd, c);
}

void Quectel_LG76::poll() {
  switch (state) {
    case S_WAIT_LEN:
      if (millis() >= tReady) {
        wire.requestFrom(ADDR_DATA, (uint8_t)4);
        uint32_t v = 0;
        for (int i=0; i<4 && wire.available(); ++i) v |= uint32_t(wire.read()) << 8*i;
        availLen = v;
        busLocked = false;
        state = S_IDLE;
      }
      break;

    case S_WAIT_PAYLOAD:
      if (millis() >= tReady) {
        wire.requestFrom(ADDR_DATA, (uint8_t)rxLen);
        for (uint32_t i=0; i<rxLen && wire.available(); ++i)
          rxBuf[i] = wire.read();
        busLocked = false;
        state = S_IDLE;
      }
      break;

    case S_WAIT_WRITE:
      if (millis() >= tReady) {
        wire.beginTransmission(ADDR_WRITE);
        wire.write((const uint8_t*)txCmd, txLen);
        wire.endTransmission();
        busLocked = false;
        state = S_IDLE;
      }
      break;

    case S_IDLE:
    default:
      break;
  }
}

bool Quectel_LG76::startLengthQuery() {
  if (busLocked) return false;
  uint8_t cmd[8];
  putLE32(cmd,    0xAA510008);
  putLE32(cmd+4,  0x00000004);
  wire.beginTransmission(ADDR_CTRL);
  wire.write(cmd,8);
  wire.endTransmission();
  busLocked = true;
  state = S_WAIT_LEN;
  tReady = millis() + 10;
  return true;
}

bool Quectel_LG76::startPayloadRead(uint8_t *dst, uint32_t len) {
  if (busLocked || len==0 || len > I2C_BUF_MAX) return false;
  uint8_t cmd[8];
  putLE32(cmd,    0xAA512000);
  putLE32(cmd+4,  len);
  wire.beginTransmission(ADDR_CTRL);
  wire.write(cmd,8);
  wire.endTransmission();
  rxBuf = dst; rxLen = len;
  busLocked = true;
  state = S_WAIT_PAYLOAD;
  tReady = millis() + 10;
  return true;
}

bool Quectel_LG76::startWrite(const char *cmdStr) {
  if (busLocked) return false;
  size_t len = strlen(cmdStr);
  if (len == 0 || len > I2C_BUF_MAX) return false;
  uint8_t header[8];
  putLE32(header,   0xAA531000);
  putLE32(header+4, len);
  wire.beginTransmission(ADDR_CTRL);
  wire.write(header,8);
  wire.endTransmission();
  txCmd = cmdStr; txLen = len;
  busLocked = true;
  state = S_WAIT_WRITE;
  tReady = millis() + 10;
  return true;
}

// Blocking convenience functions
uint32_t Quectel_LG76::readAvailableLength() {
  startLengthQuery();
  while (!isIdle()) { poll(); yield(); }
  return availLen;
}

bool Quectel_LG76::readNMEA(uint8_t *buf, uint32_t maxLen, uint32_t &outLen) {
  uint32_t length = readAvailableLength();
  if (length == 0 || length > maxLen) return false;
  startPayloadRead(buf, length);
  while (!isIdle()) { poll(); yield(); }
  outLen = length;
  return true;
}

bool Quectel_LG76::sendPMTK(const char *bareCmd) {
  char tmp[128];
  makePMTK(bareCmd, tmp);
  startWrite(tmp);
  while (!isIdle()) { poll(); yield(); }
  return true;
}

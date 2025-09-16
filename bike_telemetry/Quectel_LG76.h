#ifndef _QUECTEL_LG76_H_
#define _QUECTEL_LG76_H_

#include <Arduino.h>
#include <Wire.h>

class Quectel_LG76 {
  public:
    explicit Quectel_GPS(TwoWire &w = Wire);

    void begin();
    void poll();

    // Non-blocking actions
    bool startLengthQuery();
    bool startPayloadRead(uint8_t *dst, uint32_t len);
    bool startWrite(const char *cmd);

    // Blocking convenience APIs
    uint32_t readAvailableLength();
    bool readNMEA(uint8_t *buf, uint32_t maxLen, uint32_t &outLen);
    bool sendPMTK(const char *bareCmd);

    bool isIdle() const { return state == S_IDLE && !busLocked; }

  private:
    // Quectel-specified I2C addresses
    static constexpr uint8_t ADDR_CTRL = 0x50;
    static constexpr uint8_t ADDR_DATA = 0x54;
    static constexpr uint8_t ADDR_WRITE = 0x58;

    enum State { S_IDLE, S_WAIT_LEN, S_WAIT_PAYLOAD, S_WAIT_WRITE };

    State state;
    TwoWire &wire;
    bool busLocked;
    uint32_t tReady;
    uint32_t availLen, rxLen;
    uint8_t *rxBuf;
    const char *txCmd;
    uint32_t txLen;

    static constexpr uint32_t I2C_BUF_MAX = 4096;
    void putLE32(uint8_t *dst, uint32_t v);
    uint8_t calcChecksum(const char *cmd);
    void makePMTK(const char *cmd, char *out);
};


#endif // _QUECTEL_LG76_H_
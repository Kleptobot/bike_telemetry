#ifndef LC76G_H
#define LC76G_H

#include <Arduino.h>
#include <Wire.h>
#include <queue>
#include <vector>

class LC76G
{
  public:
    // Unified state machine
    enum State {
      STATE_IDLE,
      STATE_STEP1A_SEND,
      STATE_STEP1A_DELAY,
      STATE_STEP1B_READ,
      STATE_STEP1B_DELAY,
      STATE_STEP2A_SEND,
      STATE_STEP2A_DELAY,
      STATE_STEP2B_TRANSACT,
      STATE_COMPLETE_RECEIVE,
      STATE_COMPLETE_TRANSMIT,
      STATE_ERROR
    };

    // Constructor
    LC76G();
    
    // Initialize the GPS module
    bool begin(TwoWire* wire = &Wire);
    
    // Non-blocking update - call frequently in loop()
    State update();
    
    // Get received data (only valid after STATUS_SUCCESS from receive)
    uint16_t getReceivedData(uint8_t* buffer, uint16_t maxLength);
    
    // Queue a command to send (non-blocking)
    bool queueCommand(const uint8_t* data, uint16_t length);
    
    // Check if I2C bus is busy
    bool isBusy() const { return (_state != STATE_IDLE) && (_state != STATE_ERROR); }

    //tell class that another device just performed i2c
    void i2c_wait() {_lastI2cAction = millis();};

  private:
    // I2C addresses
    static const uint8_t ADDR_CR_OR_CW = 0x50;
    static const uint8_t ADDR_READ = 0x54;
    static const uint8_t ADDR_WRITE = 0x58;
    
    // Protocol constants
    static const uint16_t CMD_READ = 0xAA51;
    static const uint16_t CMD_WRITE = 0xAA53;
    static const uint8_t CMD_LENGTH = 8;
    
    static const uint16_t REG_TX_LEN = 0x08;
    static const uint16_t REG_TX_BUF = 0x2000;
    static const uint16_t REG_RX_LEN = 0x04;
    static const uint16_t REG_RX_BUF = 0x1000;
    
    static const uint16_t MAX_BUFFER = 64;
    static const uint8_t MAX_READ_SIZE = 64; // nRF52840 I2C buffer limit

    static const uint8_t i2c_DELAY = 20;

    struct TxCommand {
      std::vector<uint8_t> data;
      TxCommand(const uint8_t* buf, uint16_t len) : data(buf, buf + len) {}


    };
    
    // Operation mode
    enum Mode {
      MODE_RECEIVE,
      MODE_TRANSMIT
    };
  
    State _state, _state_prev;
    Mode _mode;
    unsigned long _stateEntry, _lastI2cAction;
    TwoWire* _wire;
    
    // Buffers
    uint8_t _rxBuffer[MAX_BUFFER];
    uint16_t _rxLength;
    uint8_t _txBuffer[MAX_BUFFER];
    uint16_t _txLength;
    std::queue<TxCommand> _txQueue;
    
    uint8_t _lengthBytes[4];
    uint8_t _cmdBuffer[CMD_LENGTH];
    uint16_t _transactionLength;
    uint8_t _errorCount = 0;
    
    // Helpers
    void setState(State newState);
    void setDelayMs(uint16_t ms);
    bool isDelayComplete();
    bool i2cWrite(uint8_t addr, const uint8_t* data, uint16_t length);
    bool i2cRead(uint8_t addr, uint8_t* data, uint16_t length);
    bool sendReadRequest(uint16_t reg, uint16_t length);
    bool sendWriteRequest(uint16_t reg, uint16_t length);
    bool readResponse(uint8_t* buffer, uint16_t length);
    bool writeData(const uint8_t* buffer, uint16_t length);
    int Recovery_I2c();

};

#endif
#ifndef LC76G_H
#define LC76G_H

#include <Arduino.h>
#include <Wire.h>
#include <queue>
#include <vector>
#include <TinyGPSPlus.h>
#include "HAL/StorageInterface.hpp"

static const uint8_t MAX_NMEA_LEN = 255;

class LC76G
{
    private:

    struct Sentence {
        char data[MAX_NMEA_LEN];
        uint16_t length;

        Sentence(){}
        Sentence(uint8_t* buff, uint8_t len) {
            memcpy(data, buff, len);
            length = len;
        }
    };

    static int build_no_args(char* buf, uint16_t size, const char* cmd, const void*) {
        return snprintf(buf, size, "%s", cmd);
    }

    static int build_1u8(char* buf, uint16_t size, const char* cmd, const void* payload) {
        const Payload1U8* p = static_cast<const Payload1U8*>(payload);

        return snprintf(buf, size, "%s,%u", cmd, p->a);
    }

    static int build_1char_1u8(char* buf, uint16_t size, const char* cmd, const void* payload) {
        const Payload1Ch1U8* p = static_cast<const Payload1Ch1U8*>(payload);

        return snprintf(buf, size, "%s,%s,%u", cmd, p->a, p->b);
    }

    static int build_1char(char* buf, uint16_t size, const char* cmd, const void* payload) {
        const Payload1Ch* p = static_cast<const Payload1Ch*>(payload);

        return snprintf(buf, size, "%s,%s", cmd, p->a);
    }

    static int build_2u8(char* buf, uint16_t size, const char* cmd, const void* payload) {
        const Payload2U8* p = static_cast<const Payload2U8*>(payload);

        return snprintf(buf, size, "%s,%u,%u", cmd, p->a, p->b);
    }

    static bool decode_none(Sentence response, int& numArgs, void* payload) {
        numArgs = 0;
        return true; 
    }

    static bool decode_1u8(Sentence response, int& numArgs, void* payload) {
        uint8_t* byte_array = (uint8_t*)payload;
        numArgs = sscanf(response.data,"%*[^,],%hu*%*X",&byte_array[0]);
	    return numArgs==2;
    }

    static bool decode_2u8(Sentence response, int& numArgs, void* payload) {
        uint8_t* byte_array = (uint8_t*)payload;
        numArgs = sscanf(response.data,"%*[^,],%hu,%hu*%*X",&byte_array[0],&byte_array[1]);
	    return numArgs==2;
    }

    static bool decode_1char_1u8(Sentence response, int& numArgs, void* payload) {
      Payload1Ch1U8* p = (Payload1Ch1U8*)payload;
      numArgs = sscanf(response.data,"%*[^,],%s,%hu*%*X",&p->a,&p->b);
	    return numArgs==2;
    }


    public:
      LC76G(IStorage& storage) 
      : _state(STATE_IDLE),
        _state_prev(STATE_IDLE),
        _mode(MODE_RECEIVE),
        _stateEntry(millis()),
        _lastI2cAction(millis()),
        _wire(nullptr),
        _storage(storage),
        _rxLength(0),
        _transactionLength(0) {}

        //enum of all avaialble commands
        enum CmdId : int16_t {
            PAIR_LOW_POWER_ENTRY_RTC_MODE,
            GET_NMEA_MSG_RATE,
            SET_NMEA_MSG_RATE,
            GET_NAVIGATION_MODE,
            SET_NAVIGATION_MODE,
            GET_STATIC_THRESHOLD,
            SET_STATIC_THRESHOLD,
            RESTORE_DEFAULT_SETTING,
            GET_GLP_STATUS,
            GLP_ENABLE,
            GNSS_SUBSYS_HOT_START,
            GNSS_SUBSYS_WARM_START,
            GNSS_SUBSYS_COLD_START,
            SET_NMEA_RATE,
            GET_NMEA_RATE
        };

        struct CommandDef {
            CmdId             cmdId;    // logical ID

            const char* cmd;            // command body to send
            int (*build)(               // builder to use for this command
                char* outBuf,
                uint16_t outSize,
                const char* cmd,
                const void* payload
            );                            

            const char* response;       // response body for matching against
            bool (*decode)(             // decoder to use for this response
                Sentence resp,
                int& numArgs,
                void* payload         
            );
            uint32_t          timeoutMs;  
        };

        inline static const CommandDef COMMANDS[] = {
            {PAIR_LOW_POWER_ENTRY_RTC_MODE, "PAIR650", &build_no_args, nullptr, nullptr, 200 },
            {GET_NMEA_MSG_RATE, "PAIR063", &build_1u8, "$PAIR063", &decode_2u8, 90000 },
            {SET_NMEA_MSG_RATE, "PAIR062,", &build_2u8, "PAIR001,062", &decode_1u8, 90000 },
            {GET_NAVIGATION_MODE, "PAIR081", &build_no_args, "$PAIR081", &decode_1u8, 90000 },
            {SET_NAVIGATION_MODE, "PAIR080,", &build_1u8, "$PAIR001,080", &decode_none, 90000 },
            {GET_STATIC_THRESHOLD, "PAIR071", &build_no_args, "$PAIR071", &decode_1u8, 90000 },
            {SET_STATIC_THRESHOLD, "PAIR070", &build_1u8, "$PAIR001,070", &decode_none, 90000 },
            {RESTORE_DEFAULT_SETTING, "PAIR514", &build_no_args, nullptr, nullptr, 200 },
            {GET_GLP_STATUS, "PAIR681", &build_no_args, "$PAIR681", &decode_1u8, 90000 },
            {GLP_ENABLE, "PAIR680", &build_1u8, "PAIR001,680", &decode_none, 90000 },
            {GNSS_SUBSYS_HOT_START, "PAIR004", &build_no_args, nullptr, nullptr, 200 },
            {GNSS_SUBSYS_WARM_START, "PAIR005", &build_no_args, nullptr, nullptr, 200 },
            {GNSS_SUBSYS_COLD_START, "PAIR006", &build_no_args, nullptr, nullptr, 200 },
            {SET_NMEA_RATE, "PQTMCFGMSGRATE,W", &build_1char_1u8, "$PQTMCFGMSGRATE,OK", nullptr, 10000 },
            {GET_NMEA_RATE, "PQTMCFGMSGRATE,R", &build_1char, "$PQTMCFGMSGRATE,OK", nullptr, 10000 }

        };

        using ResponseCallback = void (*)(int numArgs, const void* payload, void* userContext);

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
      STATE_PROCESS_RECEIVE,
      STATE_COMPLETE_RECEIVE,
      STATE_COMPLETE_TRANSMIT,
      STATE_ERROR
    };

    struct PendingResponse {
      CmdId commandId;
      ResponseCallback callback;
      void* userContext;
      uint32_t sendTimeMs;
      uint32_t timeoutMs;
    };

    struct Payload1U8 {
      uint8_t a;
    };

    struct Payload2U8 {
      uint8_t a;
      uint8_t b;
    };

    struct Payload1Ch1U8 {
      char a[10];
      uint8_t b;
    };

    struct Payload1Ch {
      char a[10];
    };
    
    // Initialize the GPS module
    bool begin(TwoWire* wire = &Wire);
    
    // Non-blocking update
    void update();

    // main state machine
    State stateMachine();

    // Get received data (only valid after STATUS_SUCCESS from receive)
    uint16_t getReceivedData(uint8_t* buffer, uint16_t maxLength);
    
    // Queue a command to send (non-blocking)
    bool queueCommand(const uint8_t* data, uint16_t length);
    
    // Check if I2C bus is busy
    bool isBusy() const { return (_state != STATE_IDLE) && (_state != STATE_ERROR); }

    //tell class that another device just performed i2c
    void i2c_wait() {_lastI2cAction = millis();};
    void sendCommand(CmdId cmdId, ResponseCallback cb, void* userCtx, const void* payload);

    TinyGPSPlus& gps() { return _gps; };
    void closeDataFile() {
      dataFile.flush();
      dataFile.close();
      delay(50);
    }

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

    static const uint8_t i2c_DELAY = 10;

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
    
    TinyGPSPlus _gps;
    IStorage& _storage;
    File32 dataFile;
    
    // Buffers
    uint8_t _rxBuffer[MAX_BUFFER];
    uint16_t _rxLength;
    uint8_t _txBuffer[MAX_BUFFER];
    uint8_t _NMEAbuffer[MAX_NMEA_LEN];
    uint16_t _NMEAlength;
    uint16_t _txLength;
    std::queue<TxCommand> _txQueue;
    std::queue<Sentence> _sentences;
    std::vector<PendingResponse> _responses;
    
    uint8_t _lengthBytes[4];
    uint8_t _cmdBuffer[CMD_LENGTH];
    uint16_t _transactionLength;
    uint8_t _errorCount = 0;

    bool _CR = false;
    bool _$found = false;
    
    // Helpers
    bool i2cWrite(uint8_t addr, const uint8_t* data, uint16_t length);
    bool i2cRead(uint8_t addr, uint8_t* data, uint16_t length);
    bool sendReadRequest(uint16_t reg, uint16_t length);
    bool sendWriteRequest(uint16_t reg, uint16_t length);
    bool readResponse(uint8_t* buffer, uint16_t length);
    bool writeData(const uint8_t* buffer, uint16_t length);
    int Recovery_I2c();

    // Functions for handling protocol
    uint8_t calculate_xor_checksum(const uint8_t* data, size_t length);
    void addSentence();
    void pollResponseTimeouts();
    void processSentence(Sentence s);

    uint16_t getCommand(CmdId cmdId) {
      for (uint16_t i =0; i< std::size(COMMANDS); ++i) {
        if ( COMMANDS[i].cmdId == cmdId) return i;
      }
      return 0;
    }
};

#endif
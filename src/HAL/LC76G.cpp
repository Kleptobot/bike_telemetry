#include "LC76G.hpp"
#include <numeric>
#include <string.h>
#include <cstring>

LC76G::LC76G() 
  : _state(STATE_IDLE),
    _state_prev(STATE_IDLE),
    _mode(MODE_RECEIVE),
    _stateEntry(millis()),
    _lastI2cAction(millis()),
    _wire(nullptr),
    _rxLength(0),
    _transactionLength(0) {
}

bool LC76G::begin(TwoWire* wire) {
  _wire = wire;
  _state = STATE_IDLE;
  return true;
}

void LC76G::pollResponseTimeouts() {
  uint32_t now = millis();

  for (auto it = _responses.begin();it != _responses.end(); ) {
    if ((now - it->sendTimeMs) > it->timeoutMs) {
      Serial.println("timeout");
      if(it->callback)
        it->callback(0, nullptr, it->userContext);        // call the callback but with no 
      it = _responses.erase(it);
    } else {
      ++it;
    }
  }
}

void LC76G::processSentence(Sentence s) {
    Serial.write(s.data,s.length);

    for (auto it = _responses.begin(); it != _responses.end(); ++it) {
        uint16_t index = getCommand(it->commandId);

        if (std::strncmp(s.data, COMMANDS[index].response, std::strlen(COMMANDS[index].response)) == 0) {
            //decode the response
            uint8_t buffer[5];
            int numArgs;
            if(COMMANDS[index].decode(s, numArgs, buffer)) {
                // Fire callback
                it->callback(numArgs, buffer, it->userContext);
            }
      
            // Remove expectation
            _responses.erase(it);
            return;
        }
    }

    // Not a response → normal telemetry handling
    for (uint16_t i = 0; i < s.length; ++i) {
        _gps.encode(s.data[i]);
    }
}

void LC76G::update() {
  stateMachine();

  pollResponseTimeouts();

  if(!_sentences.empty()){
    processSentence(_sentences.front());
    _sentences.pop();
  }
}

LC76G::State LC76G::stateMachine() {
  uint16_t reg;

  switch (_state) {
    
    case STATE_IDLE:
      if (_errorCount > 20) {
        _errorCount = 0;
        Recovery_I2c();
      }
      if (millis() - _lastI2cAction > i2c_DELAY) {
        if(_txQueue.empty()) {
          _mode = MODE_RECEIVE;
        }else{
          TxCommand& cmd = _txQueue.front();
          memset(_txBuffer,0,MAX_BUFFER);
          memcpy(_txBuffer, cmd.data.data(), cmd.data.size());
          Serial.write(_txBuffer,cmd.data.size());
          _txLength = cmd.data.size();
          _txQueue.pop();
          _mode = MODE_TRANSMIT;
        }
        _state=STATE_STEP1A_SEND;
      }
      break;
    
    case STATE_STEP1A_SEND:
      // Step 1a: Request length/buffer size register
      reg = REG_TX_LEN;
      if (_mode != MODE_RECEIVE) {
        reg = REG_RX_LEN;
      }
      // Ask: "How much data is available to read?"
      if (sendReadRequest(reg, 4)) {
        _state=STATE_STEP1A_DELAY;
      } else {
        _state=STATE_ERROR;
      }
      break;
    
    case STATE_STEP1A_DELAY:
      if (millis() - _stateEntry > i2c_DELAY)
        _state=STATE_STEP1B_READ;
      break;
    
    case STATE_STEP1B_READ:
      // Step 1b: Read length/buffer size
      if (i2cRead(ADDR_READ, _lengthBytes, 4)) {
        uint16_t length = _lengthBytes[0] | (_lengthBytes[1] << 8);
        
        if (_mode == MODE_RECEIVE) {
          if (length == 0) {
            _state=STATE_IDLE;
            return _state;
          }
          // Limit to max read size
          _transactionLength = min(length, (uint16_t)MAX_BUFFER);
        } else {
          // MODE_TRANSMIT - check buffer size
          if (_txLength > length) {
            _state=STATE_ERROR;
            return _state;
          }
          _transactionLength = _txLength;
        }
        
        _state=STATE_STEP1B_DELAY;
      } else {
        _state=STATE_ERROR;
      }
      break;
    
    case STATE_STEP1B_DELAY:
      if (millis() - _stateEntry > i2c_DELAY)
        _state=STATE_STEP2A_SEND;
      break;
    
    case STATE_STEP2A_SEND:
      // Step 2a: Request buffer access
      if (_mode == MODE_RECEIVE) {
        // Say: "I want to read N bytes from TX buffer"
        if (sendReadRequest(REG_TX_BUF, _transactionLength)) {
          _state=STATE_STEP2A_DELAY;
        } else {
          _state=STATE_ERROR;
        }
      } else {
        // Say: "I want to write N bytes to RX buffer"
        if (sendWriteRequest(REG_RX_BUF, _transactionLength)) {
          _state=STATE_STEP2A_DELAY;
        } else {
          _state=STATE_ERROR;
        }
      }
      break;
    
    case STATE_STEP2A_DELAY:
      if (millis() - _stateEntry > i2c_DELAY)
        _state = STATE_STEP2B_TRANSACT;
      break;
    
    case STATE_STEP2B_TRANSACT:
      // Step 2b: Read or write data
      if (_mode == MODE_RECEIVE) {
        memset(_rxBuffer, 0, sizeof(_rxBuffer));
        if (readResponse(_rxBuffer, _transactionLength)) {
          _state=STATE_PROCESS_RECEIVE;
        } else {
          _state=STATE_ERROR;
        }
      } else {
        if (writeData(_txBuffer, _transactionLength)) {
          if (_transactionLength>0)_state=STATE_COMPLETE_TRANSMIT;
          else _state=STATE_COMPLETE_RECEIVE;
        } else {
          _state=STATE_ERROR;
        }
      }
      break;
    
    case STATE_PROCESS_RECEIVE:
      _CR = false;
      for(int i=0; i<_transactionLength;i++){
        if (_rxBuffer[i] == '$') _$found = true;                // check for '$'
        if(_$found) _NMEAbuffer[_NMEAlength++] = _rxBuffer[i];  // push char to assembly buffer (only if '$' has been seen)
        if(_rxBuffer[i] == '\n') {                              // LF found
          if (_CR) {
            addSentence();                                      // add sentence to queue
          } else {
            _CR = false;                                        // CR not found
          }
        };  
        if(_rxBuffer[i] == '\r') _CR = true;                    // CR found
        else _CR = false;                                       // CR not found
      }
      
      _state=STATE_COMPLETE_RECEIVE;
      break;
    
    case STATE_COMPLETE_RECEIVE:
      if (millis() - _stateEntry > i2c_DELAY)
        _state=STATE_IDLE;
      break;

    case STATE_COMPLETE_TRANSMIT:
      if (millis() - _stateEntry > i2c_DELAY)
        _state=STATE_IDLE;
      break;
    
    case STATE_ERROR:
      if (_mode == MODE_RECEIVE) {
        _rxLength = 0;
      }
      if (millis() - _stateEntry > i2c_DELAY) {
        _errorCount +=1;
        _state=STATE_IDLE;
      }
      break;
  }

  if(_state != _state_prev) {
    _stateEntry = millis();
  }
  _state_prev = _state;

  return _state;
}

uint16_t LC76G::getReceivedData(uint8_t* buffer, uint16_t maxLength) {
  uint16_t copyLength = min(_transactionLength, maxLength);
  if (copyLength > 0) {
    memcpy(buffer, _rxBuffer, copyLength);
  }
  return copyLength;
}

bool LC76G::queueCommand(const uint8_t* data, uint16_t length) {
  if (length > MAX_BUFFER) {
    return false;
  }

  _txQueue.emplace(data, length);
  
  return true;
}

bool LC76G::i2cWrite(uint8_t addr, const uint8_t* data, uint16_t length) {
  _wire->beginTransmission(addr);
  _wire->write(data, length);
  return (_wire->endTransmission() == 0);
}

bool LC76G::i2cRead(uint8_t addr, uint8_t* data, uint16_t length) {
  uint16_t received = _wire->requestFrom(addr, length);
  if (received != length) {
    return false;
  }
  
  for (uint16_t i = 0; i < length; i++) {
    data[i] = _wire->read();
  }
  return true;
}

// High-level helper: Send a read request to a register
bool LC76G::sendReadRequest(uint16_t reg, uint16_t length) {
  // Build command: CMD_READ | register | length
  uint32_t requestCmd[2];
  requestCmd[0] = ((uint32_t)CMD_READ << 16) | reg;
  requestCmd[1] = length;
  
  return i2cWrite(ADDR_CR_OR_CW, (uint8_t*)requestCmd, CMD_LENGTH);
}

bool LC76G::sendWriteRequest(uint16_t reg, uint16_t length) {
  // Build command: CMD_WRITE | register | length
  uint32_t requestCmd[2];
  requestCmd[0] = ((uint32_t)CMD_WRITE << 16) | reg;
  requestCmd[1] = length;
  
  return i2cWrite(ADDR_CR_OR_CW, (uint8_t*)requestCmd, CMD_LENGTH);
}

// High-level helper: Read a response from the module
bool LC76G::readResponse(uint8_t* buffer, uint16_t length) {
  return i2cRead(ADDR_READ, buffer, length);
}

// High-level helper: Write data to the module
bool LC76G::writeData(const uint8_t* buffer, uint16_t length) {
  return i2cWrite(ADDR_WRITE, buffer, length);
}

int LC76G::Recovery_I2c() {
    uint8_t dummy_data = 0;
    if (i2cWrite(ADDR_CR_OR_CW, &dummy_data, 1)) {
        return 1;
    }

    if (i2cWrite(ADDR_READ, &dummy_data, 1)) {
        return 2;
    }

    if (i2cWrite(ADDR_WRITE, &dummy_data, 1)) {
        return 3;
    }
    return 0;
}

/**
 * Calculates the 8-bit XOR checksum for a byte array.
 * @param data Pointer to the array of unsigned characters (bytes).
 * @param length The length of the array.
 * @return The calculated 8-bit checksum.
 */
uint8_t LC76G::calculate_xor_checksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; ++i) {
        checksum ^= data[i]; // XOR each byte with the running total
    }
    return checksum;
}

/**
 * formats a command for sending to the LC76G
 * if present queues an expected response
 * @param cmdId command to send
 * @param cb callback to fire on response
 * @param userCtx context of the response
 * @param payload payload to send with the cmd
 */
void LC76G::sendCommand(CmdId cmdId, ResponseCallback cb = nullptr, void* userCtx = nullptr, const void* payload = nullptr) {
    char buffer[64];  // allow for body + framing
    int length = 0;

    uint16_t index = getCommand(cmdId);

    //call this command's build function
    length = COMMANDS[index].build(buffer,sizeof(buffer),COMMANDS[index].cmd,payload);

    // Compute checksum over the command body only (no '$', '*', or CRLF)
    uint8_t checksum = calculate_xor_checksum(reinterpret_cast<const uint8_t*>(buffer), length);

    // Format full NMEA sentence
    char buffer2[64];  // allow for body + framing
    length = sprintf(buffer2, "$%s*%02X\r\n", buffer, checksum);

    queueCommand(reinterpret_cast<const uint8_t*>(buffer2), length);

    _responses.push_back({
      cmdId,
      cb,
      userCtx,
      millis(),
      COMMANDS[index].timeoutMs
    });
}

/**
 * check for well formed sentence in assembly area, iff well formed add to sentence buffer
 */
void LC76G::addSentence() {
  if(_NMEAlength > 3) {
    if(_NMEAbuffer[0] == '$' && _NMEAbuffer[_NMEAlength-2] == '\r' && _NMEAbuffer[_NMEAlength-1] == '\n') { //check sentence bounds
      _sentences.emplace(_NMEAbuffer, _NMEAlength); // push sentence to queue
      memset(_NMEAbuffer, 0, sizeof(_NMEAbuffer));  // clear assembly buffer
      _$found = false;                              // reset if $ found
      _NMEAlength = 0;                              // reset sentence length
    }
  }
}
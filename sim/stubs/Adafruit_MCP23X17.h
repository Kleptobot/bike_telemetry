#ifndef ADAFRUIT_MCP23X17_H_STUB
#define ADAFRUIT_MCP23X17_H_STUB
#include "Arduino.h"

// Port A bit assignments, taken from InputSystem::update's shifts. The
// simulator composes this byte from its button state; the real InputSystem
// then does its own edge detection and hold timing on top, so button
// behaviour (press vs hold vs repeat) is the firmware's, not the stub's.
#define SIM_MCP_BIT_RIGHT   3
#define SIM_MCP_BIT_DOWN    4
#define SIM_MCP_BIT_SELECT  5
#define SIM_MCP_BIT_LEFT    6
#define SIM_MCP_BIT_UP      7
#define SIM_MCP_BIT_SD_DET  2   // inverted: 0 = card present

void    simSetMcpGPIOA(uint8_t value);
uint8_t simGetMcpGPIOA();

class Adafruit_MCP23X17 {
public:
    bool begin_I2C(uint8_t = 0x20) { return true; }
    void setupInterrupts(bool, bool, uint8_t) {}
    void setupInterruptPin(uint8_t, uint8_t) {}
    void pinMode(uint8_t, uint8_t) {}
    void digitalWrite(uint8_t pin, bool v) { _out[pin & 0x0F] = v; }
    bool digitalRead(uint8_t pin) { return _out[pin & 0x0F]; }
    uint8_t readGPIOA() { return simGetMcpGPIOA(); }
    void clearInterrupts() {}
private:
    bool _out[16] = {false};
};
#endif

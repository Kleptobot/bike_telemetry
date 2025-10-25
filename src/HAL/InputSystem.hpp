#ifndef _INPUTSYSTEM_H_
#define _INPUTSYSTEM_H_

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <queue>
#include <functional>

#include "button.hpp"
#include "InputInterface.hpp"

#define MCP23017_ADDR 0x20

#define WAKEUP_PIN D1

#define GPIOA0 0
#define GPIOA1 1
#define GPIOA2 2
#define GPIOA3 3
#define GPIOA4 4
#define GPIOA5 5
#define GPIOA6 6
#define GPIOA7 7

#define GPIOB0 8
#define GPIOB1 9
#define GPIOB2 10
#define GPIOB3 11
#define GPIOB4 12
#define GPIOB5 13
#define GPIOB6 14
#define GPIOB7 15

enum class ButtonID : uint8_t {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  SELECT,
  SD_DET,
  COUNT // internal use
};

class InputSystem {
public:
  InputSystem() {
    buttons[static_cast<size_t>(ButtonID::UP)] = new button(&bUp);
    buttons[static_cast<size_t>(ButtonID::DOWN)] = new button(&bDown);
    buttons[static_cast<size_t>(ButtonID::LEFT)] = new button(&bLeft);
    buttons[static_cast<size_t>(ButtonID::RIGHT)] = new button(&bRight);
    buttons[static_cast<size_t>(ButtonID::SELECT)] = new button(&bSelect);
    buttons[static_cast<size_t>(ButtonID::SD_DET)] = new button(&bSD_Det);
  }
  void init();
  bool update(bool i2cBusy);

  const physIO state() const;
  void setOutput(uint8_t PIN, bool state);

  using GpsEnableCallback = std::function<void(bool pinState)>;

  static void onEnableStateRead(GpsEnableCallback cb) { gpsEnableCallback = cb; }

private:
    struct pinCMD {
      uint8_t pin;
      bool cmd;
    };
    static const uint16_t MCP_Period = 97;
    Adafruit_MCP23X17 _mcp;
    bool bUp, bDown, bLeft, bRight, bSelect, bSD_Det;
    uint32_t lastMCPTime = 0;
    button* buttons[static_cast<size_t>(ButtonID::COUNT)];
    std::queue<pinCMD> pinCmds;

    static GpsEnableCallback gpsEnableCallback;
};

#endif // _INPUTSYSTEM_H_
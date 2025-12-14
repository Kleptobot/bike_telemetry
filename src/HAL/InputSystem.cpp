#include "InputSystem.hpp"

InputSystem::GpsEnableCallback InputSystem::gpsEnableCallback;

void InputSystem::init() {
// Set up the sense mechanism to generate the DETECT signal to wake from system_off
  pinMode(WAKEUP_PIN, INPUT_PULLDOWN_SENSE);  // this pin (WAKE_HIGH_PIN) is pulled down and wakes up the feather when externally connected to 3.3v.
  pinMode(D2, INPUT);
  pinMode(D6, OUTPUT);
  
  digitalWrite(D6, false);    //turn off the auxilary supply

  if (!_mcp.begin_I2C()) {
    Serial.println("MCP init Error");
  }

  _mcp.setupInterrupts(true, false, HIGH);  //configure interrupts
  _mcp.pinMode(GPIOA2, INPUT_PULLUP);
  _mcp.pinMode(GPIOA3, INPUT_PULLUP);
  _mcp.pinMode(GPIOA4, INPUT_PULLUP);
  _mcp.pinMode(GPIOA5, INPUT_PULLUP);
  _mcp.pinMode(GPIOA6, INPUT_PULLUP);
  _mcp.pinMode(GPIOA7, INPUT_PULLUP);
  _mcp.pinMode(GPIOB3, OUTPUT);
  _mcp.pinMode(GPIOB4, OUTPUT);
  _mcp.pinMode(GPIOB5, OUTPUT);
  _mcp.pinMode(GPIOB6, OUTPUT);
  _mcp.pinMode(GPIOB7, OUTPUT);
  _mcp.setupInterruptPin(GPIOA5, HIGH);

  _mcp.digitalWrite(GPIOB3, false);   //turn the gps backup supply off
  _mcp.digitalWrite(GPIOB4, true);    //turn the gps enable supply on
  
  _mcp.digitalWrite(GPIOB5, true);    //set the reset pin of the gps
  _mcp.digitalWrite(GPIOB6, false);   //turn the display off
  //_mcp.digitalWrite(GPIOB7, true);    //set the reset pin of the display

}

bool InputSystem::update(bool i2cBusy) {
    bool update = false;
    if ((millis() - lastMCPTime > MCP_Period) && !i2cBusy) {
        lastMCPTime = millis();
        uint8_t currentA = _mcp.readGPIOA();

        bUp = (currentA >> 7) & 0x01;
        bDown = (currentA >> 4) & 0x01;
        bLeft = (currentA >> 6) & 0x01;
        bRight = (currentA >> 3) & 0x01;
        bSelect = (currentA >> 5) & 0x01;
        bSD_Det = (currentA >> 2) & 0x01;
        _mcp.clearInterrupts();

        //run through the queue of pin cmds
        while (!pinCmds.empty()) {
          _mcp.digitalWrite(pinCmds.front().pin, pinCmds.front().cmd);
          pinCmds.pop();
        }
        if (gpsEnableCallback) {
          gpsEnableCallback(_mcp.digitalRead(GPIOB3));
        }

        update = true;
    }

    for (size_t i = 0; i<static_cast<size_t>(ButtonID::COUNT); i ++) {
      buttons[i]->process();
    }
    return update;
}

physIO const InputSystem::state() const {
  return {buttons[static_cast<size_t>(ButtonID::UP)]->state(),
          buttons[static_cast<size_t>(ButtonID::DOWN)]->state(),
          buttons[static_cast<size_t>(ButtonID::LEFT)]->state(),
          buttons[static_cast<size_t>(ButtonID::RIGHT)]->state(),
          buttons[static_cast<size_t>(ButtonID::SELECT)]->state(),
          buttons[static_cast<size_t>(ButtonID::SD_DET)]->state()};  
}

void InputSystem::setOutput(uint8_t PIN, bool state) {
  pinCmds.push({PIN, state});
}
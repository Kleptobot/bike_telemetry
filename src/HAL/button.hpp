#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <Arduino.h>
#include "InputInterface.hpp"

class button
{
  private:
    bool* _state;
    bool _state_prev;
    bool _RE, _FE;
    bool _long, _short;
    uint32_t _RETime = 0, _FETime = 0;
    uint32_t _heldTime;
    uint32_t _releaseTime;
    static const uint16_t _nHeldPressTime = 1500;
    static const uint16_t _nShortPressTime = 400;

  public:
    button(bool* state)
    {
      _state = state;
    };

    void process();

    const buttonState state() const {
      return {*_state,_RE,_FE,_short,_long,_heldTime,_releaseTime};
    }
};

#endif // _BUTTON_H_
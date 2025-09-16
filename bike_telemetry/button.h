#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <Arduino.h>

class button
{
  private:
    bool* _state;
    bool _state_prev;
    bool _RE, _FE;
    bool _long, _short, _seen, _short_held;
    uint16_t _mult;
    uint64_t _pressTime, _nHeldPressTime, _nShortPressTime;

  public:
    button(bool* state, uint64_t nHeldTime, uint64_t nShortTime)
    {
      _state = state;
      _nHeldPressTime = nHeldTime;
      _nShortPressTime = nShortTime;
    };

    void process();

    bool state(){return _state;};
    bool RE(){return _RE;};
    bool FE(){return _FE;};
    bool shortPress(){return _short;};
    bool shortHeld(){return _short_held;};
    bool longHeld(){return _long;};
    uint16_t mult(){return _mult;};
};

#endif // _BUTTON_H_
#include "button.h"

void button::process()
{
  uint64_t current_time = millis();
  
  _RE = _state && !_state_prev;
  _FE = !_state && _state_prev;

  if (_RE)
    _pressTime = current_time;

  _long = _state && ((current_time - _pressTime) > _nHeldPressTime);
  _mult = (current_time - _pressTime) / _nHeldPressTime;

  if(_short)
    _short=false;

  _short = _FE && ((current_time - _pressTime) < _nShortPressTime) && _seen;
  _short_held = _state && !_long && ((current_time - _pressTime) > _nShortPressTime);

  if(!_state){
    _seen = false;
    _mult = 0;
  }

  _state_prev = _state;
};
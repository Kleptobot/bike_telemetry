#include "button.hpp"

void button::process()
{
  //get current time, millis is good enough for timing button presses
  uint32_t current_time = millis();

  //reset an calculated values
  _short = false;

  //check for rising falling
  _RE = *_state && !_state_prev;
  _FE = !*_state && _state_prev;

  //capture time of rising edge
  if (_RE)
    _RETime = current_time;

  //capture time of falling edge
  if (_FE)
    _FETime = current_time;

  //based on state and captured edge times calculate the time held/released
  //also check if a short press has been performed
  if (*_state) {
    _heldTime = current_time - _RETime;
    _releaseTime = 0;
  } else {
    if (_heldTime<_nShortPressTime)
      _short = true;
    _heldTime = 0;
    _releaseTime = current_time - _FETime;
  }

  _long = _heldTime > _nHeldPressTime;

  _state_prev = *_state;
};
#ifndef _TIME_MENU_H_
#define _TIME_MENU_H_

#include <Arduino.h>
#include <RTClib.h>

#include "HAL/button.hpp"

class TimeMenu
{
  private:
    button* _Up = nullptr;
    button* _Down = nullptr;
    button* _Left = nullptr;
    button* _Right = nullptr;
    button* _Center = nullptr;

    int16_t s16TimeFocusVert;
    int16_t s16TimeFocusHoz;
    int16_t s16TimeFocus;
    int16_t s16DateFocus;
    int16_t s16NavFocus;
    bool selected;

  public:

    TimeMenu(button* Up, button* Down, button* Left, button* Right, button* Center)
    {
      _Up = Up;
      _Down = Down;
      _Left = Left;
      _Right = Right;
      _Center = Center;
    };

    DateTime updateTime(DateTime someTime);
    void drawDateTime(DateTime someTime, int x, int y);
    void timeSelection();

    bool back(){return s16NavFocus == 1 && selected;};
    bool save(){return s16NavFocus == 2 && selected;};


};

#endif //_TIME_MENU_H_
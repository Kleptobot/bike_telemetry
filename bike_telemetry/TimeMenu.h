#ifndef _TIME_MENU_H_
#define _TIME_MENU_H_

#include <Arduino.h>
#include <RTClib.h>
#include "button.h"

class TimeMenu
{
  private:
    button* _Up = nullptr;
    button* _Down = nullptr;
    button* _Left = nullptr;
    button* _Right = nullptr;
    button* _Center = nullptr;

    int16_t s16timeSel;
    bool bHourSel, bMinuteSel, bSecondSel;
    bool bHourFoc, bMinuteFoc, bSecondFoc;
    bool bDaySel, bMonthSel, bYearSel;
    bool bDayFoc, bMonthFoc, bYearFoc;
    bool bBackFoc, bBackSel, bSaveFoc, bSaveSel;

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

    bool back(){return bBackSel;};
    bool save(){return bSaveSel;};


};

#endif //_TIME_MENU_H_
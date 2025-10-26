#ifndef _BODY_H_
#define _BODY_H_

#include <Arduino.h>
#include <RTClib.h>

#include "HAL/button.hpp"

class body
{
  private:
    int16_t nStatSel;
    int nMassDisplay;
    bool bMassSel, bMassFoc, bAgeFoc, bAgeSel, bStatBackSel, bStatBackFoc;
    bool bDaySel, bMonthSel, bYearSel;
    bool bDayFoc, bMonthFoc, bYearFoc;
    DateTime _bday;

    button* _Up = nullptr;
    button* _Down = nullptr;
    button* _Left = nullptr;
    button* _Right = nullptr;
    button* _Center = nullptr;

  public:
    body(button* Up, button* Down, button* Left, button* Right, button* Center)
    {
      _Up = Up;
      _Down = Down;
      _Left = Left;
      _Right = Right;
      _Center = Center;
    };

    void statSelection();
    void drawStats(int x, int y);
    
    void setBday(DateTime bday){_bday=bday;};
    TimeSpan getAge(DateTime now){return now-_bday;};

    void setMass(int nMass){nMassDisplay = nMass;};
    int getMass(){return nMassDisplay;};

};

#endif
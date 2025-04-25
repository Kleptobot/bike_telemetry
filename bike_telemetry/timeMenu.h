#ifndef TIMEMENU_H
#define TIMEMENU_H

#include "GUI.h"
#include "GFX.h"

namespace timeMenu{
  menu_item hours, minutes, seconds;
  menu_item days, months, years;
  menu_item save, back;

  menu items({{&hours,&minutes,&seconds},
                {&days,&minutes,&years},
                {&save,&back}});

  /**
  * Determines what value in the data time display is focused or slected
  */
  void timeSelection() {
    //button inputs
    if (!items.selected()) {
      if (UP.shortPress())
        items.decVert();

      if (DOWN.shortPress()) 
        items.incVert();

      if (LEFT.shortPress())
        items.decHoz();

      if (RIGHT.shortPress())
        items.incHoz();
    }

    if(CENTER.shortPress())
      items.toggleSel();
  }

  void drawDateTime(DateTime someTime, int x, int y) {
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(2);
    display.setCursor(x, y);

    //print hours
    drawSelectable(x+2, y+2, someTime.hour(), hours.focus, hours.select);
    display.print(":");
    //print minutes
    drawSelectable(display.getCursorX(), display.getCursorY(), someTime.minute(), minutes.focus, minutes.select);
    display.print(":");
    //print seconds
    drawSelectable(display.getCursorX(), display.getCursorY(), someTime.second(), seconds.focus, minutes.select);

    ///add some padding
    display.setCursor(x+2, display.getCursorY() + 24);

    //print days
    drawSelectable(display.getCursorX(), display.getCursorY(), someTime.day(), days.focus, days.select);
    display.print("/");
    //print months
    drawSelectable(display.getCursorX(), display.getCursorY(), someTime.month(), months.focus, months.select);
    display.print("/");
    //print years
    drawSelectable(display.getCursorX(), display.getCursorY(), someTime.year(), years.focus, years.select);

    ///add some more padding
    display.setCursor(x + 16, display.getCursorY() + 24);

    ///print the menu options
    drawSelectable(x, display.getCursorY(), epd_bitmap_left, "Back", back.focus, back.select);
    drawSelectable(display.getCursorX(), display.getCursorY(), epd_bitmap_save, "Save", save.focus, save.select);
  }

  DateTime updateTime(DateTime someTime) {
    int8_t s8AddVal;
    if (UP.shortPress()) {
      s8AddVal = 1;
    } else if (DOWN.shortPress()) {
      s8AddVal = -1;
    }

    TimeSpan span;
    if (seconds.select) {
      DateTime dt0(someTime.year(), someTime.month(), someTime.day(), someTime.hour(), someTime.minute(), someTime.second() + s8AddVal);
      someTime = dt0;
    } else if (minutes.select) {
      DateTime dt0(someTime.year(), someTime.month(), someTime.day(), someTime.hour(), someTime.minute() + s8AddVal, someTime.second());
      someTime = dt0;
    } else if (hours.select) {
      DateTime dt0(someTime.year(), someTime.month(), someTime.day(), someTime.hour() + s8AddVal, someTime.minute(), someTime.second());
      someTime = dt0;
    } else if (days.select) {
      DateTime dt0(someTime.year(), someTime.month(), someTime.day() + s8AddVal, someTime.hour(), someTime.minute(), someTime.second());
      someTime = dt0;
    } else if (months.select) {
      DateTime dt0(someTime.year(), someTime.month() + s8AddVal, someTime.day(), someTime.hour(), someTime.minute(), someTime.second());
      someTime = dt0;
    } else if (years.select) {
      DateTime dt0(someTime.year() + s8AddVal, someTime.month(), someTime.day(), someTime.hour(), someTime.minute(), someTime.second());
      someTime = dt0;
    }

    return someTime;
  }
};

#endif
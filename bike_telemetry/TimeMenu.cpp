#include "TimeMenu.h"
#include "GUI.h"

DateTime TimeMenu::updateTime(DateTime someTime) {
  int8_t s8AddVal;
  if (_Up->shortPress()) {
    s8AddVal = 1;
  } else if (_Down->shortPress()) {
    s8AddVal = -1;
  }

  TimeSpan span;
  if (bSecondSel) {
    DateTime dt0(someTime.year(), someTime.month(), someTime.day(), someTime.hour(), someTime.minute(), someTime.second() + s8AddVal);
    someTime = dt0;
  } else if (bMinuteSel) {
    DateTime dt0(someTime.year(), someTime.month(), someTime.day(), someTime.hour(), someTime.minute() + s8AddVal, someTime.second());
    someTime = dt0;
  } else if (bHourSel) {
    DateTime dt0(someTime.year(), someTime.month(), someTime.day(), someTime.hour() + s8AddVal, someTime.minute(), someTime.second());
    someTime = dt0;
  } else if (bDaySel) {
    DateTime dt0(someTime.year(), someTime.month(), someTime.day() + s8AddVal, someTime.hour(), someTime.minute(), someTime.second());
    someTime = dt0;
  } else if (bMonthSel) {
    DateTime dt0(someTime.year(), someTime.month() + s8AddVal, someTime.day(), someTime.hour(), someTime.minute(), someTime.second());
    someTime = dt0;
  } else if (bYearSel) {
    DateTime dt0(someTime.year() + s8AddVal, someTime.month(), someTime.day(), someTime.hour(), someTime.minute(), someTime.second());
    someTime = dt0;
  }

  return someTime;
}

void TimeMenu::timeSelection() {
  //button inputs
  if (!(bHourSel | bMinuteSel | bSecondSel | bDaySel | bMonthSel | bYearSel)) {
    if (_Up->shortPress()) {
      if ((s16timeSel & 0x0300) >= 0x0100)
        s16timeSel -= 0x0100;
      else
        s16timeSel += 0x0200;
    }

    if (_Down->shortPress()) {
      if ((s16timeSel & 0x0200) < 0x0200)
        s16timeSel += 0x100;
      else
        s16timeSel -= 0x0200;
    }

    if (_Left->shortPress()) {
      if ((s16timeSel & 0x0030) >= 0x0010)
        s16timeSel -= 0x0010;
      else
        s16timeSel += 0x0020;
    }

    if (_Right->shortPress()) {
      if ((s16timeSel & 0x0020) < 0x0020)
        s16timeSel += 0x0010;
      else
        s16timeSel &= 0xFFCF;
    }
  }

  if (s16timeSel > 0x211) {
    s16timeSel &= 0xFF0F;
  }

  if (_Center->shortPress())
    s16timeSel = s16timeSel ^ 1;

  //calculate boolean values
  bHourFoc = (s16timeSel == 0x000);
  bHourSel = (s16timeSel == 0x001);
  bMinuteFoc = (s16timeSel == 0x010);
  bMinuteSel = (s16timeSel == 0x011);
  bSecondFoc = (s16timeSel == 0x020);
  bSecondSel = (s16timeSel == 0x021);

  bDayFoc = (s16timeSel == 0x100);
  bDaySel = (s16timeSel == 0x101);
  bMonthFoc = (s16timeSel == 0x110);
  bMonthSel = (s16timeSel == 0x111);
  bYearFoc = (s16timeSel == 0x120);
  bYearSel = (s16timeSel == 0x121);

  bBackFoc = (s16timeSel == 0x200);
  bBackSel = (s16timeSel == 0x201);
  bSaveFoc = (s16timeSel == 0x210);
  bSaveSel = (s16timeSel == 0x211);
}


/**
 * Draws the date time 
 * @param someTime the date time to display
 * @param x top left x coordinate of the date time
 * @param y top left y coordinate of the date time
 */
void TimeMenu::drawDateTime(DateTime someTime, int x, int y) {
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(x, y);

  //print hours
  drawSelectable(x+2, y+2, someTime.hour(), bHourFoc, bHourSel);
  display.print(":");
  //print minutes
  drawSelectable(display.getCursorX(), display.getCursorY(), someTime.minute(), bMinuteFoc, bMinuteSel);
  display.print(":");
  //print seconds
  drawSelectable(display.getCursorX(), display.getCursorY(), someTime.second(), bSecondFoc, bSecondSel);

  ///add some padding
  display.setCursor(x+2, display.getCursorY() + 24);

  //print days
  drawSelectable(display.getCursorX(), display.getCursorY(), someTime.day(), bDayFoc, bDaySel);
  display.print("/");
  //print months
  drawSelectable(display.getCursorX(), display.getCursorY(), someTime.month(), bMonthFoc, bMonthSel);
  display.print("/");
  //print years
  String years = String(someTime.year());
  drawSelectable(display.getCursorX(), display.getCursorY(), years.c_str(), bYearFoc, bYearSel);

  ///add some more padding
  display.setCursor(x + 16, display.getCursorY() + 24);

  ///print the menu options
  drawSelectable(x, display.getCursorY(), epd_bitmap_left, "Back", bBackFoc, bBackSel);
  drawSelectable(display.getCursorX(), display.getCursorY(), epd_bitmap_save, "Save", bSaveFoc, bSaveSel);
}
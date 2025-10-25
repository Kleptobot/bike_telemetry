#include "TimeMenu.hpp"
#include "GUI.hpp"

DateTime TimeMenu::updateTime(DateTime someTime) {
  int32_t s32AddVal = 0;
 
  
  if(s32AddVal != 0)
  {
    if (s16TimeFocus == 1 && selected) {
      TimeSpan span(0,0,0,s32AddVal);
      someTime = someTime + span;
    } else if (s16TimeFocus == 2 && selected) {
      TimeSpan span(0,0,s32AddVal,0);
      someTime = someTime + span;
    } else if (s16TimeFocus == 3 && selected) {
      TimeSpan span(0,0,s32AddVal,0);
      someTime = someTime + span;

    } else if (s16DateFocus == 1 && selected) {
      TimeSpan span(s32AddVal,0,0,0);
      someTime = someTime + span;
    } else if (s16DateFocus == 2 && selected) {
      DateTime dt0(someTime.year(), someTime.month() + s32AddVal, someTime.day(), someTime.hour(), someTime.minute(), someTime.second());
      someTime = dt0;
    } else if (s16DateFocus == 3 && selected) {
      DateTime dt0(someTime.year() + s32AddVal, someTime.month(), someTime.day(), someTime.hour(), someTime.minute(), someTime.second());
      someTime = dt0;
    }
  }

  return someTime;
}

void TimeMenu::timeSelection() {
  //button inputs
  


  if(s16TimeFocusVert == 0)
    s16TimeFocus = s16TimeFocusHoz;
  else
    s16TimeFocus = 0;

  if(s16TimeFocusVert == 1)
    s16DateFocus = s16TimeFocusHoz;
  else
    s16DateFocus = 0;

  if(s16TimeFocusVert == 2)
    s16NavFocus = s16TimeFocusHoz;
  else
    s16NavFocus = 0;
  
  if(s16NavFocus>2)
    s16NavFocus=2;


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

  drawTime(x+2, y+2, someTime, s16TimeFocus, selected);

  ///add some padding
  display.setCursor(x+2, display.getCursorY() + 24);

  drawDate(display.getCursorX(), display.getCursorY(), someTime, s16DateFocus, selected);

  ///add some more padding
  display.setCursor(x + 16, display.getCursorY() + 24);

  ///print the menu options
  drawSelectable(x, display.getCursorY(), epd_bitmap_left, "Back", s16NavFocus==1, s16NavFocus==1 && selected);
  drawSelectable(display.getCursorX(), display.getCursorY(), epd_bitmap_save, "Save", s16NavFocus==2, s16NavFocus==2 && selected);
}
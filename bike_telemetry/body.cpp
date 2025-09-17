#include "body.h"

void body::statSelection(){
  if(!bAgeSel && !bMassSel && !bStatBackSel){
    if(_Up->shortPress()){
      if((nStatSel & 0x30) >= 0x10)
        nStatSel -= 0x10;
      else
        nStatSel += 0x20;

    }else if(_Down->shortPress()){
      if((nStatSel & 0x20) < 0x20)
        nStatSel += 0x10;
      else
        nStatSel -= 0x20;
    }
  } else if (bDaySel) {
    DateTime dt0(_bday.year(), _bday.month(), _bday.day() + 1, _bday.hour(), _bday.minute(), _bday.second());
    _bday = dt0;
  } else if (bMonthSel) {
    DateTime dt0(_bday.year(), _bday.month() + 1, _bday.day(), _bday.hour(), _bday.minute(), _bday.second());
    _bday = dt0;
  } else if (bYearSel) {
    DateTime dt0(_bday.year() + 1, _bday.month(), _bday.day(), _bday.hour(), _bday.minute(), _bday.second());
    _bday = dt0;
  }else if(bMassSel){
    if(_Up->shortPress())
      nMassDisplay ++;
    else if(_Down->shortPress())
      nMassDisplay --;
  }
  if(_Center->shortPress())
    nStatSel = nStatSel ^ 1;

  bAgeFoc = (nStatSel==0x00);
  bAgeSel = (nStatSel==0x01);
  bMassFoc = (nStatSel==0x10);
  bMassSel = (nStatSel==0x11);
  bStatBackFoc = (nStatSel==0x20);
  bStatBackSel = (nStatSel==0x21);

  if(bStatBackSel){
    //tcxLog.setAge(nAgeDisplay);
    //tcxLog.setMass(nMassDisplay);
    nStatSel = 0;
    //u16_state = 1;
    bStatBackSel = bStatBackFoc =false;
  }
}


void body::drawStats(int x, int y){
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(x, y);

  display.print("Age: ");
  display.setCursor(x, display.getCursorY()+16);
  //print day
  drawSelectable(display.getCursorX(), display.getCursorY(), _bday.day(), bDayFoc, bDaySel);
  display.print("/");
  //print month
  drawSelectable(display.getCursorX(), display.getCursorY(), _bday.month(), bMonthFoc, bMonthSel);
  display.print("/");
  //print year
  String years = String(_bday.year());
  drawSelectable(display.getCursorX(), display.getCursorY(), years.c_str(), bYearFoc, bYearSel);



  display.print("Mass: ");
  drawSelectable(64, display.getCursorY(), nMassDisplay, bMassFoc, bMassSel);
  display.setCursor(x, display.getCursorY()+32);
  drawSelectable(x, display.getCursorY(), epd_bitmap_left, "Back", bStatBackFoc, bStatBackSel);
  
}
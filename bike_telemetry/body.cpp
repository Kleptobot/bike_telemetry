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
  }else if(bAgeSel){
    if(_Up->shortPress())
      nAgeDisplay ++;
    else if(_Down->shortPress())
      nAgeDisplay --;
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
  drawSelectable(64, display.getCursorY(), nAgeDisplay, bAgeFoc, bAgeSel);
  display.setTextSize(2);
  display.setCursor(x, display.getCursorY()+16);
  display.print("Mass: ");
  drawSelectable(64, display.getCursorY(), nMassDisplay, bMassFoc, bMassSel);
  display.setCursor(x, display.getCursorY()+32);
  drawSelectable(x, display.getCursorY(), epd_bitmap_left, "Back", bStatBackFoc, bStatBackSel);
}
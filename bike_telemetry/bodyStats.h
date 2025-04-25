#ifndef BODYSTATS_H
#define BODYSTATS_H

#include "GUI.h"
#include "GFX.h"

namespace bodyStats {

  struct param {
    const char name[32];
    uint32_t val;
    menu_item state;
  };

  param stats[]={
    {"Age",35,{false,false}},
    {"Mass",74,{false,false}},
  };
  menu_item back,save;
  
  int nAgeDisplay, nMassDisplay;

  //lambda function to dynamically create a vector of the menu_items in the PM2_settings array
  std::vector<menu_item*> menuVector = [] {
    std::vector<menu_item*> vec;
    for (param& setting : stats) {
      vec.push_back(&setting.state);
    }
    vec.push_back(&save);
    vec.push_back(&back);
    return vec;
  }();

  menu items({menuVector});

  void drawStats(int x, int y){
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(2);
    display.setCursor(x, y);

    display.print("Age: ");
    drawSelectable(64, display.getCursorY(), nAgeDisplay, stats[0].state.focus, stats[0].state.select);
    display.setTextSize(2);
    display.setCursor(x, display.getCursorY()+16);
    display.print("Mass: ");
    drawSelectable(64, display.getCursorY(), nMassDisplay, stats[1].state.focus, stats[1].state.select);
    display.setCursor(x, display.getCursorY()+32);
    drawSelectable(x, display.getCursorY(), epd_bitmap_left, "Save", save.focus, save.select);
    display.setCursor(x, display.getCursorY()+32);
    drawSelectable(x, display.getCursorY(), epd_bitmap_left, "Back", back.focus, back.select);
  }
  
  void statSelection(){
    if(!items.selected()){
      if(UP.shortPress()){
        items.decVert();
      }else if(DOWN.shortPress()){
        items.incVert();
      }
    }

    if(CENTER.shortPress())
      items.toggleSel();
  }


}

#endif
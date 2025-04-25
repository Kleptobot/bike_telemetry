#ifndef SETTINGS_H
#define SETTINGS_H

#include "GUI.h"
#include "GFX.h"

namespace settingsMenu{
  struct SettingsItem {
      const unsigned char* img;
      int x;
      int y;
      char name[32];
      menu_item state;
  };

  SettingsItem SettingsItems[] = {
    {epd_bitmap_clock_large,32,32,"Time",{false, false}},
    {epd_bitmap_bluetooth_large,32,32,"BLE",{false, false}},
    {epd_bitmap_heart_large,32,32,"Info",{false, false}},
    {epd_bitmap_antenna_large,32,32,"GPS",{false, false}},
    {epd_bitmap_left_arrow_large,32,32,"Back",{false, false}}
  };

  //lambda function to dynamically create a vector of the menu_items in the PM2_settings array
  std::vector<menu_item*> menuVector = [] {
    std::vector<menu_item*> vec;
    for (SettingsItem& setting : SettingsItems) {
      vec.push_back(&setting.state);
    }
    return vec;
  }();

  menu items({menuVector});

  void settingsSelect(){
    if (UP.shortPress())
      items.decVert();

    if (DOWN.shortPress())
      items.incVert(); 
  }

  void drawSettingsmenuItem(int x, int y, int16_t menuIndex) {
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(2);
    display.setCursor(x, y);
  

    int16_t x1, y1;
    uint16_t w, h;

    display.getTextBounds(SettingsItems[menuIndex].name, x, y, &x1, &y1, &w, &h);
    
    display.drawBitmap(x+2, y+1, SettingsItems[menuIndex].img, SettingsItems[menuIndex].x, SettingsItems[menuIndex].y, 1);
    display.setCursor(x+32+8, y+8);
    display.print(SettingsItems[menuIndex].name);
    if (SettingsItems[menuIndex].state.focus)
      display.drawRect(x, y, 127, 34, 1);
  }

  void drawSettings(){

    display.setTextColor(SH110X_WHITE);

    int16_t aboveIndex = items.vert()-1;
    if(items.vert() == 0)
      aboveIndex = items.vertLen()-1;

    int16_t belowIndex = items.vert()+1;
    if(items.vert() == (items.vertLen()-1))
      belowIndex = 0;

    drawSettingsmenuItem(0, 13, aboveIndex);
    drawSettingsmenuItem(0, 47, items.vert());
    drawSettingsmenuItem(0, 81, belowIndex);

  }
}
#endif
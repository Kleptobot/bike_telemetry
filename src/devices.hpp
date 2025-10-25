
#include <Adafruit_SH110X.h>

#define nDeviceWindowLen 2


extern Adafruit_SH1107 display;

typedef struct
{
  char name[32];
  uint8_t MAC[6];
  bool stored;
  uint16_t batt;
  E_Type_BT_Device type;
} prph_info_t;
std::vector<prph_info_t> nearby_devices;

int16_t s16DeviceSel, s16DeviceWindowStart;
bool bBackFocDev, bBackSelDev;

void deviceSelection() {
  if (Up.shortPress()) {
    if (s16DeviceSel >= 1)
    {
      s16DeviceSel --;
      if(s16DeviceSel < s16DeviceWindowStart)
        s16DeviceWindowStart --;
    }
    Serial.print("s16DeviceSel = ");
    Serial.println(s16DeviceSel);
    Serial.print("s16DeviceWindowStart = ");
    Serial.println(s16DeviceWindowStart);
  }

  if (Down.shortPress()) {
    if (s16DeviceSel < nearby_devices.size())
      s16DeviceSel ++;
      if((s16DeviceSel >= (s16DeviceWindowStart + nDeviceWindowLen)) && (s16DeviceSel < nearby_devices.size()))
        s16DeviceWindowStart++;
    Serial.print("s16DeviceSel = ");
    Serial.println(s16DeviceSel);
    Serial.print("s16DeviceWindowStart = ");
    Serial.println(s16DeviceWindowStart);
  }

  if (s16DeviceSel != nearby_devices.size()) {
    if (Center.shortPress()) {
      nearby_devices[s16DeviceSel].stored = !nearby_devices[s16DeviceSel].stored;

      BT_Device* device = BT_Device::getDeviceWithMAC(nearby_devices[s16DeviceSel].MAC);
      if (device != NULL && !nearby_devices[s16DeviceSel].stored) {
          Bluefruit.disconnect(device->getConnHandle());
          BT_Device::removeDeviceWithMAC(nearby_devices[s16DeviceSel].MAC);
      }else if (device == NULL && nearby_devices[s16DeviceSel].stored){
        Serial.println("Adding device:");
        Serial.print("Name: ");
        Serial.println(nearby_devices[s16DeviceSel].name);
        Serial.print("MAC: ");
        Serial.printBufferReverse(nearby_devices[s16DeviceSel].MAC, 6, ':');
        Serial.print("\n");
        switch(nearby_devices[s16DeviceSel].type)
        {
          case E_Type_BT_Device::bt_csc:
            csc::create_csc(nearby_devices[s16DeviceSel].name, 32, nearby_devices[s16DeviceSel].MAC);
            break;
          case E_Type_BT_Device::bt_hrm:
            hrm::create_hrm(nearby_devices[s16DeviceSel].name, 32, nearby_devices[s16DeviceSel].MAC);
            break;
          case E_Type_BT_Device::bt_cps:
            cps::create_cps(nearby_devices[s16DeviceSel].name, 32, nearby_devices[s16DeviceSel].MAC);
            break;
        }
      }
    }
  }

  bBackFocDev = s16DeviceSel == nearby_devices.size();
  bBackSelDev = Center.shortPress() && bBackFocDev;
}

void showDevices() {

  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 8);
  display.setTextSize(1);
  display.print("Bluetooth Devices");

  int devicesToShow = nDeviceWindowLen;
  if(nearby_devices.size() < nDeviceWindowLen)
    devicesToShow = nearby_devices.size();
  else if(s16DeviceSel>(nearby_devices.size()+nDeviceWindowLen))
    s16DeviceSel--;

  if (nearby_devices.size() > 0) {
    //iterate the list
    for (int i = 0; i < devicesToShow; i++) {
      bool selected, focus, discovered;
      uint8_t batt;

      focus = s16DeviceSel == i+s16DeviceWindowStart;

      BT_Device* device = BT_Device::getDeviceWithMAC(nearby_devices[i+s16DeviceWindowStart].MAC);
      if (device != NULL) {
        discovered = device->discovered();
        batt = device->readBatt();
      }
      DrawDevice(2, i * 28 + 20 + 2, nearby_devices[i+s16DeviceWindowStart], focus, nearby_devices[i+s16DeviceWindowStart].stored, discovered, batt);
    }
  }

  ///print the menu options
  drawSelectable(3, 95, epd_bitmap_left, "Back", bBackFocDev, bBackSelDev);
};

void DrawDevice(int x, int y, prph_info_t device, bool focus, bool stored, bool discovered, uint16_t batt) {
  display.setTextSize(1);
  display.setCursor(x, y);
  String tempString = device.name;

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(tempString, x, y, &x1, &y1, &w, &h);

  display.setTextColor(SH110X_WHITE);
  if (focus)
    display.drawRect(x - 2, y - 3, 127, 30, 1);
  display.setCursor(x, y);
  display.print(tempString);
  display.drawBitmap(x, y + 10, epd_bitmap_down_right, 16, 16, 1);

  if (device.stored)
  {
    display.drawBitmap(x + 18, y + 10, epd_bitmap_save, 16, 16, 1);

    if (discovered)
      display.drawBitmap(x + 34, y + 10, epd_bitmap_Bluetooth, 16, 16, 1);

    display.drawBitmap(x + 50, y + 10, epd_bitmap_battery, 32, 16, 1);
    display.setCursor(x + 57, y + 15);
  }
  display.print(batt);
}
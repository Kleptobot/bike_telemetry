// #include <Arduino.h>
// #include <LSM6DS3.h>
// #include <Wire.h>
// #include <SPI.h>
// #include "SdFat.h"
// #include "sdios.h"
// #include <bluefruit.h>
// #include <RTClib.h>
// #include <xiaobattery.h>
// #include <nrf52840.h>
// #include <Adafruit_MCP23X17.h>
// #include <Dps3xx.h>
// #include <ArduinoJson.h>
// #include <TinyGPSPlus.h>
// #include <vector>

// #include "Utils.hpp"
// #include "UI/GFX.h"
// #include "GUI.hpp"
// #include "TimeMenu.hpp"

// #define MCP23017_ADDR 0x20
// Adafruit_MCP23X17 mcp;

// #define WAKEUP_PIN D1
// #define GPIOA0 0
// #define GPIOA1 1
// #define GPIOA2 2
// #define GPIOA3 3
// #define GPIOA4 4
// #define GPIOA5 5
// #define GPIOA6 6
// #define GPIOA7 7

// #define GPIOB0 8
// #define GPIOB1 9
// #define GPIOB2 10
// #define GPIOB3 11
// #define GPIOB4 12
// #define GPIOB5 13
// #define GPIOB6 14
// #define GPIOB7 15

// #define Battery_Read_Period 10000
// #define GPS_Read_Period 1000
// #define Timeout_Period 60000
// #define nIMUReadPeriod 500
// #define nDSPReadPeriod 10000
// #define nStartDelayPeriod 3000
// #define nLoadDevicesDelay 3000
// #define nDeviceScanDelay 500

// #define nNumStats 2

// TinyGPSPlus gps;
// uint8_t rxBuffer[128];

// //Create a instance of class LSM6DS3
// LSM6DS3 myIMU(I2C_MODE, 0x6A);  //I2C device address 0x6A

// int16_t s16SettingsSel, s16GPS_SettingsSel;

// float f32_RTC_Temp, f32_DSP_Temp, f32_DSP_Pa, f32_Alt, f32_acc_x, f32_acc_y, f32_acc_z, f32_gyro_x, f32_gyro_y, f32_gyro_z, f32_kph, f32_cadence, f32_bpm, f32_kph_last, f32_distance;
// float f32_GPS_speed, f32_GPS_angle, f32_GPS_Alt;
// double f32_GPS_long, f32_GPS_lat;
// uint32_t nGPS_Hrs, nGPS_Min, nGPS_Sec;

// bool b_Running, b_Running_Prev;

// Xiao battery;
// float fBatteryVoltage;
// int nBatteryPercentage;

// DateTime dtTimeDisplay;

// bool bSysOff;

// uint16_t u16_state, u16_state_prev;
// bool bStateEntry = true;
// bool paused = false;

// SdFat32 SD;
// //RTC
// RTC_DS3231 rtc;

// DateTime nCurrentTime;
// bool bSwitchOnDelay;
// bool started, bDevicesLoaded;
// bool sleepGPS;

// uint32_t startMillis, lastSecond, lastMillis, stateEntryMillis, nLastAction;
// uint32_t lastMCPTime = 0, lastRTCTime = 0, lastDSPTime = 0, lastIMUTime = 0, lastBATTime = 0;
// uint64_t nLastSecond = 0;

// // Dps3xx Object
// Dps3xx Dps3xxPressureSensor = Dps3xx();

// uint8_t toConnectMAC[6];

// // bluefruit callback functions
// void connect_callback(uint16_t conn_handle);
// void scan_callback(ble_gap_evt_adv_report_t* report);
// void scan_discovery(ble_gap_evt_adv_report_t* report);

// //internal functions
// void readIMU();
// void drawMain();
// void GUI();

// void setup() {
//   ;
// }

// void init_devices() {
//   ;
// }

// uint8_t nDetThresh;

// void loop() {
  

//   if ((nDetThresh==20) || !started) {
//     // initialise SD card
//     if (!SD.begin(SD_CS)) {
//       Serial.println("initialisation failed.");
//       delay(500);
//     } else {
//       debugLog.open("/log.txt", FILE_WRITE);
//       Serial.println("SD card initialised");
//     }
//     nDetThresh ++;
//   }

//   //draw the display  
//   if (millis() - lastMillis > 100) {
    
//     display.clearDisplay();

//     GUI();

//     display.display();

//     lastMillis = millis();
//   }

//   //data logging
//   if (b_Running) {
//     if (nCurrentTime.secondstime() != nLastSecond) {
//       f32_distance += (f32_kph + f32_kph_last)*(0.5/3.6);
//       f32_kph_last = f32_kph;

//       // tcxLog.addTrackpoint({nCurrentTime,f32_GPS_lat,f32_GPS_long,f32_GPS_Alt,f32_bpm,0,f32_cadence,f32_kph,f32_distance});

//       nLastSecond = nCurrentTime.secondstime();
//     }
//   } else {
//     f32_kph_last = 0;
//   }

// }



// //draw the main screen
// void drawMain() {
//   //draw status symbols
//   display.setTextSize(1);
//   display.drawBitmap(95, 0, epd_bitmap_battery, 32, 16, 1);
//   display.setCursor(102, 5);
//   display.print(nBatteryPercentage);
//   if(gps.location.isValid()){
//     display.drawBitmap(0, 0, epd_bitmap_antenna, 16, 16, 1);
//   }
  
//   display.setTextSize(4);
//   display.setCursor(0, 16);
//   if (f32_kph < 10)
//     display.print(' ');
//   display.print(f32_kph, 1);
//   display.setTextSize(1);
//   display.setCursor(96, 16);
//   display.println("  k/h");

//   display.setTextSize(2);
//   display.setCursor(47, 48);

//   if (f32_cadence < 10)
//     display.print("   ");
//   else if (f32_cadence < 100)
//     display.print("  ");
//   else
//     display.print(" ");
//   display.print(f32_cadence, 0);
//   display.setTextSize(1);
//   display.setCursor(96, 48);
//   display.print("  rpm");

//   //display.drawBitmap(60, 62, epd_bitmap_heart, 16, 16, 1);
//   display.setTextSize(2);
//   display.setCursor(47, 64);
//   if(f32_bpm<10)
//     display.print("   ");
//   else if (f32_bpm < 100)
//     display.print("  ");
//   else
//     display.print(" ");
//   display.print(f32_bpm, 0);
//   display.setTextSize(1);
//   display.setCursor(96, 64);
//   display.print("  bpm");

//   if (b_Running) {
//     drawMenuRunning(64, 88);
//     display.setCursor(0, 112);
//     display.setTextSize(2);
//     // display.print(tcxLog.elapsedString_Lap());
//   } else {
//     drawMenuStopped(64, 88);
//   }
// }

// //screens
// void GUI() {

//   switch (u16_state) {
//     case 0:  //main screen
//       // if (nDeviceScanDelay < (millis() - stateEntryMillis)) {
//       //   if (!BT_Device::all_devices_discovered() && !Bluefruit.Scanner.isRunning()) {
//       //     Bluefruit.Scanner.setRxCallback(scan_callback);
//       //     Bluefruit.Scanner.filterUuid(GATT_CSC_UUID, UUID16_SVC_HEART_RATE, GATT_CPS_UUID, GATT_BAT_UUID);
//       //     Bluefruit.Scanner.useActiveScan(true);
//       //     Bluefruit.Scanner.start(0);
//       //   }
//       // }
      
//       bSwitchOnDelay = (millis() - startMillis > nStartDelayPeriod);
//       // bSysOff = Right.longHeld() && !b_Running && !bSwitchOnDelay;

//       drawMain();

     

//       break;

//     case 1:  //settings menu
//       drawSettings(s16SettingsSel);
//       break;

//     case 2:  //time

//       // dtTimeDisplay = timeMenu.updateTime(dtTimeDisplay);
//       // timeMenu.drawDateTime(dtTimeDisplay, 0, 0);
//       // timeMenu.timeSelection();
//       // ExitTime();
//       break;

//     case 3:  //human info
//       // human.drawStats(0,0);
//       // human.statSelection();
//       break;

//     case 4:  //nearby devices
//       // if (bStateEntry && !Bluefruit.Scanner.isRunning()) {
//       //   Bluefruit.Scanner.setRxCallback(scan_discovery);
//       //   Bluefruit.Scanner.filterUuid(GATT_CSC_UUID, UUID16_SVC_HEART_RATE, GATT_CPS_UUID);
//       //   Bluefruit.Scanner.useActiveScan(1);
//       //   Bluefruit.Scanner.start(0);  // // 0 = Don't stop scanning after n seconds
//       // }
//       // showDevices();
//       // deviceSelection();
//       //ExitDevices();
//       break;

//     case 5:   //gps settings
//       //drawGPSsettings();
//       //GPSSelect();
//       break;
//   }
//   //stateTransition();
//   bStateEntry = u16_state_prev != u16_state;
//   if (bStateEntry)
//   {
//     stateEntryMillis = millis();
//   }
//   u16_state_prev = u16_state;
// }

#include <LSM6DS3.h>
#include <Wire.h>
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Adafruit_SH110X.h>
#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
#include <bluefruit.h>
#include <RTClib.h>
#include <xiaobattery.h>
#include <nrf52840.h>
#include <Adafruit_MCP23X17.h>
#include <Dps3xx.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>

#include "Utils.h"
#include "BT_Device.h"
#include "csc.h"
#include "hrm.h"
#include "cps.h"
#include "GFX.h"
#include "logger.h"
#include "TCXLogger.h"
#include "uBlox.h"

#define MCP23017_ADDR 0x20
Adafruit_MCP23X17 mcp;

#define i2c_Address 0x3c   //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_HEIGHT 128  // OLED display height, in pixels
#define TFT_CS D0
#define TFT_RST -1
//#define TFT_DC        D2
#define OLED_RESET -1  //   QT-PY / XIAO
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define WAKEUP_PIN D1
#define GPIOB0 8
#define GPIOB1 9
#define GPIOB2 10
#define GPIOB3 11
#define GPIOB4 12

#define Battery_Read_Period 60
#define GPS_Read_Period 1000
#define Timeout_Period 60
#define nIMUReadPeriod 500
#define nDSPReadPeriod 10000
#define nStartDelayPeriod 3000
#define nLoadDevicesDelay 3000
#define nDeviceScanDelay 1000
#define nHeldPressTime 1500
#define nShortPressTime 400

#define nDeviceWindowLen 2

#define s16NumSettings 5
#define nNumStats 2

TinyGPSPlus gps;

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);  //I2C device address 0x6A

typedef struct
{
  char name[32];
  uint8_t MAC[6];
  bool stored;
  uint16_t batt;
  E_Type_BT_Device type;
} prph_info_t;
std::vector<prph_info_t> nearby_devices;

uint8_t toConnectMAC[6];

int16_t s16DeviceSel, s16DeviceWindowStart;
bool bBackFocDev, bBackSelDev;

int16_t s16SettingsSel, s16GPS_SettingsSel;

float f32_RTC_Temp, f32_DSP_Temp, f32_DSP_Pa, f32_Alt, f32_acc_x, f32_acc_y, f32_acc_z, f32_gyro_x, f32_gyro_y, f32_gyro_z, f32_kph, f32_cadence, f32_bpm, f32_kph_last, f32_distance;
float f32_speedSum, f32_max_speed, f32_avgSpeed, f32_cadSum, f32_max_cad, f32_avgCad, f32_bpmSum, f32_max_bpm, f32_avg_bpm;
float f32_GPS_speed, f32_GPS_angle, f32_GPS_Alt;
double f32_GPS_long, f32_GPS_lat;
uint32_t nGPS_Hrs, nGPS_Min, nGPS_Sec;

bool b_Running, b_Running_Prev;

Xiao battery;
float fBatteryVoltage;
int nBatteryPercentage;

//time selection state variables
int16_t s16timeSel;
bool bHourSel, bMinuteSel, bSecondSel;
bool bHourFoc, bMinuteFoc, bSecondFoc;
bool bDaySel, bMonthSel, bYearSel;
bool bDayFoc, bMonthFoc, bYearFoc;
bool bBackFoc, bBackSel, bSaveFoc, bSaveSel;

int16_t nStatSel;
int nAgeDisplay, nMassDisplay;
bool bMassSel, bMassFoc, bAgeFoc, bAgeSel, bStatBackSel, bStatBackFoc;

DateTime dtTimeDisplay;

//button inputs
bool bUp, bDown, bLeft, bRight, bCenter, bSD_Det;
bool bUp_Prev, bDown_Prev, bLeft_Prev, bRight_Prev, bCenter_Prev, bSD_Det_Prev;
bool bUp_RE, bDown_RE, bLeft_RE, bRight_RE, bCenter_RE, bSD_Det_RE;
bool bUp_FE, bDown_FE, bLeft_FE, bRight_FE, bCenter_FE, bSD_Det_FE;
bool bUp_long, bDown_long, bLeft_long, bRight_long, bCenter_long;
uint16_t nUp_long_mult, nDown_long_mult, nLeft_long_mult, nRight_long_mult, nCenter_long_mult;
bool bUp_short, bDown_short, bLeft_short, bRight_short, bCenter_short;
bool bUp_short_held, bDown_short_held, bLeft_short_held, bRight_short_held, bCenter_short_held;
bool bUp_Seen, bDown_Seen, bLeft_Seen, bRight_Seen, bCenter_Seen;
uint64_t nUpPress, nDownPress, nLeftPress, nRightPress, nCenterPress;
bool bSysOff;

uint16_t u16_state, u16_state_prev;
bool bStateEntry = true;

SdFat32 SD;
logger log_data = logger(500, &SD);

TCXLogger tcxLog;
bool bWriteTCX= false;

//RTC
RTC_DS3231 rtc;

DateTime nCurrentTime, nCurrentTime_Prev, nLastAction, nLastBatteryRead;
bool bSwitchOnDelay;
bool started, bDevicesLoaded;

uint32_t nMillisAtTick, nMillisAtTick_Prev, millisNow, nLastSecond, lastMillis, nStartGPS, nMillisDiff, nlastGPSUpdate;
uint64_t nCurrentTimeMillis, nStartTimeMillis, nLastIMURead, nLastDSPRead, nLastScan;

// Dps3xx Object
Dps3xx Dps3xxPressureSensor = Dps3xx();

//declare methods with default values
void drawMenuStopped(int x = 0, int y = 0);
void drawMenuRunning(int x = 0, int y = 0);

void disconnectPin(uint32_t ulPin) {
  if (ulPin >= PINS_COUNT) {
    return;
  }

  ulPin = g_ADigitalPinMap[ulPin];

  NRF_GPIO_Type* port = nrf_gpio_pin_port_decode(&ulPin);

  port->PIN_CNF[ulPin] =
    ((uint32_t)GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos)
    | ((uint32_t)GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
    | ((uint32_t)GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
    | ((uint32_t)GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
    | ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
}


void setup() {
  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 4
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  Bluefruit.begin(0, 10);

  // Set up the sense mechanism to generate the DETECT signal to wake from system_off
  pinMode(WAKEUP_PIN, INPUT_PULLDOWN_SENSE);  // this pin (WAKE_HIGH_PIN) is pulled down and wakes up the feather when externally connected to 3.3v.
  pinMode(D2, INPUT);

  Serial.begin(115200);
  GPSSerial.begin(GPSBaud);

  if (!mcp.begin_I2C()) {

    Serial.println("MCP init Error");
    while (1)
      ;
  }
  mcp.setupInterrupts(true, false, LOW);
  mcp.pinMode(GPIOB0, INPUT_PULLUP);
  mcp.pinMode(GPIOB1, INPUT_PULLUP);
  mcp.pinMode(GPIOB2, INPUT_PULLUP);
  mcp.pinMode(GPIOB3, INPUT_PULLUP);
  mcp.pinMode(GPIOB4, INPUT_PULLUP);
  mcp.setupInterruptPin(GPIOB0, HIGH);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1) delay(500);
  } else {
    Serial.println("RTC initialised");
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  nStartTimeMillis = rtc.now().unixtime() * 1000;
  nLastIMURead = nStartTimeMillis;
  nLastDSPRead = nStartTimeMillis;
  nLastScan = nStartTimeMillis;

  nMillisAtTick = 0;
  nCurrentTimeMillis = nStartTimeMillis;
  bSysOff = false;

  delay(100);
}

void init_devices() {
  display.begin(i2c_Address, true);  // Address 0x3C default
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.setRotation(3);

  display.clearDisplay();
  display.setCursor(0, 0);

  display.clearDisplay();
  display.setCursor(0, 0);
  if (myIMU.begin() != 0) {
    display.println("IMU init error");
    while (1) delay(500);
  } else {
    Serial.println("IMU initialised");
  }

  Dps3xxPressureSensor.begin(Wire);
  int16_t temp_mr = 2;
  int16_t temp_osr = 2;
  int16_t prs_mr = 2;
  int16_t prs_osr = 2;
  int16_t ret = Dps3xxPressureSensor.startMeasureBothCont(temp_mr, temp_osr, prs_mr, prs_osr);

  display.setTextColor(SH110X_WHITE);

  log_data.addSource((char*)"Temp1", &f32_RTC_Temp);
  log_data.addSource((char*)"Temp2", &f32_DSP_Temp);
  log_data.addSource((char*)"Pres", &f32_DSP_Pa);
  log_data.addSource((char*)"Alt", &f32_Alt);
  log_data.addSource((char*)"X Acc", &f32_acc_x);
  log_data.addSource((char*)"Y Acc", &f32_acc_y);
  log_data.addSource((char*)"Z Acc", &f32_acc_z);
  log_data.addSource((char*)"X Gyro", &f32_gyro_x);
  log_data.addSource((char*)"Y Gyro", &f32_gyro_y);
  log_data.addSource((char*)"Z Gyro", &f32_gyro_z);
  log_data.addSource((char*)"Batt", &fBatteryVoltage);
  log_data.addSource((char*)"Speed", &f32_kph);
  log_data.addSource((char*)"Cadence", &f32_cadence);
  log_data.addSource((char*)"Heart", &f32_bpm);

  Bluefruit.setName("OBike");

  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(250);

  // Callbacks for Central
  Bluefruit.Central.setDisconnectCallback(BT_Device::disconnect_callback);
  Bluefruit.Central.setConnectCallback(connect_callback);

  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Interval = 100 ms, window = 80 ms
   * - Don't use active scan
   * - Filter only accept csc service
   * - Start(timeout) with timeout = 0 will scan forever (until connected)
   */

  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);  // in unit of 0.625 ms

  // Minimizes power when bluetooth is used
  //NRF_POWER->DCDCEN = 1;

  started = true;
  delay(500);

  Serial.println("Booted");

  //gps wakeup
  GPSSerial.write((char)0xFF);
  uBlox_Cont();
  uBlox_Cont();
}

void loadDevices() {
  if (SD.exists("devices.txt")) {
    Serial.println("Devices file found...");
    // Open file for reading
    File32 dataFile = SD.open("/devices.txt", FILE_READ);
    // Allocate the memory pool on the stack.
    JsonDocument jsonBuffer;
    // Parse the root object

    DeserializationError error = deserializeJson(jsonBuffer, dataFile);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    for (JsonObject device : jsonBuffer["devices"].as<JsonArray>()) {

      prph_info_t newDevice;
      char name[32];
      uint8_t MAC[6];

      JsonArray device_MAC = device["MAC"];
      MAC[0] = device_MAC[0];
      MAC[1] = device_MAC[1];
      MAC[2] = device_MAC[2];
      MAC[3] = device_MAC[3];
      MAC[4] = device_MAC[4];
      MAC[5] = device_MAC[5];

      //const char* device_name = device["name"];
      device["name"].as<String>().toCharArray(name, device["name"].as<String>().length() + 1);
      device["name"].as<String>().toCharArray(newDevice.name, device["name"].as<String>().length() + 1);
      //memcpy(newDevice.name,device_name,strlen(device_name));
      copyMAC(newDevice.MAC, MAC);
      newDevice.stored = true;
      newDevice.type = device["type"];
      nearby_devices.push_back(newDevice);

      logInfo("Adding device:");
      Serial.print("Name: ");
      Serial.println(name);
      Serial.print("MAC: ");
      Serial.printBufferReverse(MAC, 6, ':');
      Serial.print("\n");
      switch(newDevice.type)
      {
        case E_Type_BT_Device::bt_csc:
          csc::create_csc(newDevice.name, 32, newDevice.MAC);
          break;
        case E_Type_BT_Device::bt_hrm:
          hrm::create_hrm(newDevice.name, 32, newDevice.MAC);
          break;
        case E_Type_BT_Device::bt_cps:
          cps::create_cps(newDevice.name, 32, newDevice.MAC);
          break;
      }
    }
    tcxLog.setAge(jsonBuffer["age"]);
    tcxLog.setMass(jsonBuffer["mass"]);

    int arrayEnd = std::end(PM2_settings)-std::begin(PM2_settings);
    for (int i=0; i<arrayEnd; i++)
    {
      PM2_settings[i].val = jsonBuffer["GPS"][i]["val"];
    }


    dataFile.close();
  }
  nLastScan = nCurrentTimeMillis;
  bDevicesLoaded = true;
}

int count = 0;
uint8_t char_buffer[2048];
uint16_t u16_buffIndex = 0;
uint32_t nCurrentMillis, nLastMillis;
uint8_t nDetThresh;

void loop() {
  uint8_t currentB;
  uint8_t pressureCount = 20;
  float pressure[pressureCount];
  uint8_t temperatureCount = 20;
  float temperature[temperatureCount];
  int16_t ret;

  //read the current time
  nCurrentTime = rtc.now();

  millisNow = millis();
  if (nCurrentTime_Prev != nCurrentTime) {
    nMillisAtTick = millisNow;
  }
  nCurrentTime_Prev = nCurrentTime;
  nCurrentTimeMillis = nCurrentTime.unixtime() * 1000 + (millisNow - nMillisAtTick);

  bSwitchOnDelay = (nStartDelayPeriod > (nCurrentTimeMillis - nStartTimeMillis));

  currentB = mcp.readGPIOB();

  bCenter = currentB & 0x01;
  bRight = (currentB >> 1) & 0x01;
  bDown = (currentB >> 2) & 0x01;
  bLeft = (currentB >> 3) & 0x01;
  bUp = (currentB >> 4) & 0x01;
  bSD_Det = digitalRead(D2);

  mcp.clearInterrupts();

  if (bRight | bUp | bDown | bCenter | bLeft) {
    nLastAction = nCurrentTime;
  }

  //check if the device has timed out
  TimeSpan ts1 = nCurrentTime - nLastAction;
  if (((ts1.totalseconds() > Timeout_Period && !b_Running) || bSysOff) && !bWriteTCX){
    if (started) {
      display.clearDisplay();
      display.display();
      uBlox_OFF();
    }
    debugLog.close();
    NRF_POWER->SYSTEMOFF = 1;
  }

  if (!bSD_Det){
    if(nDetThresh<20){
      nDetThresh ++;
    }
  }else{
    nDetThresh =0;
  }

  if ((nDetThresh==20) || !started) {
    // initialise SD card
    if (!SD.begin(SD_CS)) {
      Serial.println("initialisation failed.");
      delay(500);
    } else {
      debugLog.open("/log.txt", FILE_WRITE);
      Serial.println("SD card initialised");
    }
    nDetThresh ++;
  }
  if (bSD_Det_RE) {
    debugLog.close();
  }

  //run the rest of the init if it has not already been run
  if (!started){
    init_devices();
    Serial.println("starting");
  }

  if (!bDevicesLoaded && !bSD_Det) {
    if (nLoadDevicesDelay < (nCurrentTimeMillis - nStartTimeMillis)){
      loadDevices();
      Serial.println("loading devices");
    }
  }

  //imu read period check
  if (nIMUReadPeriod < (nCurrentTimeMillis - nLastIMURead)) {
    //get IMU data
    readIMU();
    f32_RTC_Temp = rtc.getTemperature();
    nLastIMURead = nCurrentTimeMillis;
  }

  if (nDSPReadPeriod < (nCurrentTimeMillis - nLastDSPRead)){

    ret = Dps3xxPressureSensor.getContResults(temperature, temperatureCount, pressure, pressureCount);
    //Dps3xxPressureSensor.measureTempOnce(f32_DSP_Temp, 7);
    //Dps3xxPressureSensor.measurePressureOnce(f32_DSP_Pa, 7);
    if (ret != 0)
    {
      Serial.print("FAIL! ret = ");
    }
    else
    {
      f32_DSP_Temp=0;
      for (int16_t i = 0; i < temperatureCount; i++)
      {
        f32_DSP_Temp+=temperature[i];
      }
      f32_DSP_Temp = f32_DSP_Temp/(float)temperatureCount;

      f32_DSP_Pa=0;
      for (int16_t i = 0; i < pressureCount; i++)
      {
        f32_DSP_Pa+=pressure[i];
      }
      f32_DSP_Pa = f32_DSP_Pa/(float)pressureCount;

      //estimate altitude from pressure and temperature
      float Tb = 273.15+f32_DSP_Temp;
      float P_Pb = pow(f32_DSP_Pa/101325,-0.1902663539);
      float Lb = 0.0065;
      f32_Alt = (Tb*P_Pb-Tb)/(Lb*P_Pb);
    }
    nLastDSPRead = nCurrentTimeMillis;
  }

  
    if(gps.speed.isValid()) 
      f32_GPS_speed = gps.speed.kmph();
    if(gps.altitude.isValid())
      f32_GPS_Alt = gps.altitude.meters();
    if(gps.course.isValid())
      f32_GPS_angle = gps.course.deg();
    if(gps.location.isValid()){
        f32_GPS_long = gps.location.lng();
        f32_GPS_lat = gps.location.lat();
    }
    if(gps.time.isValid()){
      nGPS_Hrs = gps.time.hour();
      nGPS_Min = gps.time.minute();
      nGPS_Sec = gps.time.second();
    }


  //battery voltage period check
  ts1 = nCurrentTime - nLastBatteryRead;
  if (ts1.totalseconds() > Battery_Read_Period) {
    //get IMU data
    fBatteryVoltage = battery.GetBatteryVoltage();
    nBatteryPercentage = fBatteryVoltage / .036;
    nLastBatteryRead = nCurrentTime;
  }

  std::vector<float> speed = csc::getSpeed();
  if(gps.speed.isValid()){
    speed.push_back(f32_GPS_speed);
  }
  f32_kph = 0;
  for (auto it = speed.begin(); it != speed.end(); it++){
    f32_kph += (*it);
  }
  if(speed.size()>0)
    f32_kph = f32_kph/speed.size();

  std::vector<float> cadence = csc::getCadence();
  f32_cadence = 0;
  for (auto it = cadence.begin(); it != cadence.end(); it++){
    f32_cadence += (*it);
  }
  if(cadence.size()>0)
    f32_cadence = f32_cadence/cadence.size();

  std::vector<float> heartrates = hrm::getHRM();
  f32_bpm = 0;
  for (auto it = heartrates.begin(); it != heartrates.end(); it++){
    f32_bpm += (*it);
  }
  if(heartrates.size()>0)
    f32_bpm = f32_bpm/heartrates.size();

  //draw the display
  millisNow = millis();
  uint32_t millisDiff = millisNow - lastMillis;
  if (millisDiff > 100) {
    
    display.clearDisplay();

    bRight_RE = bRight && !bRight_Prev && !bSwitchOnDelay;
    bRight_FE = !bRight && bRight_Prev && !bSwitchOnDelay;
    if (bRight_RE)
      nRightPress = nCurrentTimeMillis;
    bRight_long = bRight && ((nCurrentTimeMillis - nRightPress) > nHeldPressTime);
    bRight_Prev = bRight;

    bUp_RE = bUp && !bUp_Prev && !bSwitchOnDelay;
    bUp_FE = !bUp && bUp_Prev && !bSwitchOnDelay;
    if (bUp_RE)
      nUpPress = nCurrentTimeMillis;
    bUp_long = bUp && ((nCurrentTimeMillis - nUpPress) > nHeldPressTime);
    nUp_long_mult = (nCurrentTimeMillis - nUpPress) / nHeldPressTime;
    if(bUp_short)
      bUp_short=false;
    bUp_short = bUp_FE && ((nCurrentTimeMillis - nUpPress) < nShortPressTime) && bUp_Seen;
    bUp_short_held = bUp && !bUp_long && ((nCurrentTimeMillis - nUpPress) > nShortPressTime);
    if(!bUp){
      bUp_Seen = false;
      nUp_long_mult = 0;
    }
    bUp_Prev = bUp;

    bDown_RE = bDown && !bDown_Prev && !bSwitchOnDelay;
    bDown_FE = !bDown && bDown_Prev && !bSwitchOnDelay;
    if (bDown_RE)
      nDownPress = nCurrentTimeMillis;
    bDown_long = bDown && ((nCurrentTimeMillis - nDownPress) > nHeldPressTime);
    nDown_long_mult = (nCurrentTimeMillis - nDownPress) / nHeldPressTime;
    if(bDown_short)
      bDown_short=false;
    bDown_short = bDown_FE && ((nCurrentTimeMillis - nDownPress) < nShortPressTime) && bDown_Seen;
    bDown_short_held = bDown && !bDown_long && ((nCurrentTimeMillis - nDownPress) > nShortPressTime);
    if(!bDown){
      bDown_Seen = false;
      nDown_long_mult = 0;
    }
    bDown_Prev = bDown;

    bCenter_RE = bCenter && !bCenter_Prev && !bSwitchOnDelay;
    bCenter_FE = !bCenter && bCenter_Prev && !bSwitchOnDelay;
    if (bCenter_RE){
      nCenterPress = nCurrentTimeMillis;
      bCenter_Seen = true;
    }
    bCenter_long = bCenter && ((nCurrentTimeMillis - nCenterPress) > nHeldPressTime);
    if(bCenter_short)
      bCenter_short=false;
    bCenter_short = bCenter_FE && ((nCurrentTimeMillis - nCenterPress) < nShortPressTime) && bCenter_Seen;
    if(!bCenter)
      bCenter_Seen = false;
    bCenter_Prev = bCenter;

    bLeft_RE = bLeft && !bLeft_Prev && !bSwitchOnDelay;
    bLeft_FE = !bLeft && bLeft_Prev && !bSwitchOnDelay;
    if (bLeft_RE)
      nLeftPress = nCurrentTimeMillis;
    bLeft_long = bLeft && ((nCurrentTimeMillis - nLeftPress) > nHeldPressTime);
    bLeft_Prev = bLeft;

    bSD_Det_RE = bSD_Det && !bSD_Det_Prev && !bSwitchOnDelay;
    bSD_Det_FE = !bSD_Det && bSD_Det_Prev && !bSwitchOnDelay;
    bSD_Det_Prev = bSD_Det;
    GUI();

    display.display();

    lastMillis = millisNow;
  }

  nStartGPS = millis();
  nMillisDiff = 0;
  while (GPSSerial.available() && (nMillisDiff<500)){
    gps.encode(GPSSerial.read());
    nMillisDiff = millis()-nStartGPS;
  }

  //data logging
  log_data.log(nCurrentTime, millisNow - nMillisAtTick);
  if (log_data.logging()) {
    if (nCurrentTime.secondstime() > nLastSecond) {
      f32_speedSum += f32_kph;
      f32_distance += (f32_kph + f32_kph_last)*(0.5/3.6);
      f32_cadSum += f32_cadence;
      f32_bpmSum += f32_bpm;
      f32_kph_last = f32_kph;

      tcxLog.addTrackpoint({nCurrentTime,f32_GPS_lat,f32_GPS_long,f32_GPS_Alt,f32_bpm,0,f32_cadence,f32_kph,f32_distance});

      nLastSecond = nCurrentTime.secondstime();
    }
  } else {
    f32_speedSum = 0;
    f32_cadSum = 0;
    f32_bpmSum = 0;
    f32_kph_last = 0;
  }
  int32_t elapsedSeconds = log_data.elapsed().totalseconds();
  if(elapsedSeconds>0){
    f32_avgSpeed = f32_speedSum / elapsedSeconds;
    f32_avgCad = f32_cadSum / elapsedSeconds;
    f32_avg_bpm = f32_bpmSum / elapsedSeconds;
  }else{
    f32_avgSpeed = 0;
    f32_avgCad = 0;
    f32_avg_bpm = 0;
    f32_distance = 0;
  }

  if(bWriteTCX){
    if(tcxLog.finaliseLogging()){
      if (SD.remove("data.tmp")) {
        Serial.println("Deleted data.tmp");
      }
      bWriteTCX = false;
    }
  }

  //get the current max
  if(!b_Running){
    f32_max_speed = 0;
    f32_max_cad = 0;
    f32_max_bpm = 0;
  }else{
    if(f32_kph>f32_max_speed)
      f32_max_speed = f32_kph;
    if(f32_cadence>f32_max_cad)
      f32_max_cad = f32_cadence;
    if(f32_bpm>f32_max_bpm)
      f32_max_bpm = f32_bpm;
  }



}

void drawSelectable(int16_t x, int16_t y, const uint8_t* bitmap, const char* text, bool focus, bool selected) {
  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(2);
  display.getTextBounds(text, x + 16, y, &x1, &y1, &w, &h);
  if (selected) {
    display.setTextColor(SH110X_BLACK);
    display.fillRect(x - 1, y - 1, w + 17, h + 1, 1);
    display.setCursor(x + 16, y);
    display.drawBitmap(x, y, bitmap, 16, 16, 1);
    display.print(text);
  } else {
    display.setTextColor(SH110X_WHITE);
    if (focus)
      display.drawRect(x - 2, y - 2, w + 18, h + 2, 1);
    display.setCursor(x + 16, y);
    display.drawBitmap(x, y, bitmap, 16, 16, 1);
    display.print(text);
  }
  display.setTextColor(SH110X_WHITE);
}

void drawSelectable(int16_t x, int16_t y, const char* text, bool focus, bool selected) {
  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(2);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (selected) {
    display.setTextColor(SH110X_BLACK);
    display.fillRect(x - 1, y - 1, w, h, 1);
    display.setCursor(x, y);
    display.print(text);
  } else {
    display.setTextColor(SH110X_WHITE);
    if (focus)
      display.drawRect(x - 2, y - 2, w + 2, h + 2, 1);
    display.setCursor(x, y);
    display.print(text);
  }
  display.setTextColor(SH110X_WHITE);
}

void drawSelectable(int16_t x, int16_t y, int num, bool focus, bool selected) {
  int16_t x1, y1;
  uint16_t w, h;

  String text;

  if (num < 10)
    text = "0" + String(num);
  else if (num > 99)
    text = "99";
  else
    text = String(num);

  display.setTextSize(2);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (selected) {
    display.setTextColor(SH110X_BLACK);
    display.fillRect(x - 1, y - 1, w, h, 1);
    display.setCursor(x, y);
    display.print(text);
  } else {
    display.setTextColor(SH110X_WHITE);
    if (focus)
      display.drawRect(x - 2, y - 2, w + 2, h + 2, 1);
    display.setCursor(x, y);
    display.print(text);
  }
  display.setTextColor(SH110X_WHITE);
}

void drawSelectable(int16_t x, int16_t y, const char* text, int num, bool focus, bool selected) {
  int16_t x1, y1;
  uint16_t w, h;
  String numText;

  numText = String(num);
  display.getTextBounds(numText, x, y, &x1, &y1, &w, &h);

  display.setTextSize(1);  
  if (selected) {
    display.setTextColor(SH110X_BLACK);
    display.fillRect(0, y - 1, 127, 17, 1);
    display.setCursor(2, y);
    display.print(text);
    display.setCursor(128-w-2, y+8);
    display.print(numText);
  } else {
    display.setTextColor(SH110X_WHITE);
    if (focus)
      display.drawRect(0, y - 2, 128, 19, 1);
    display.setCursor(2, y);
    display.print(text);
    display.setCursor(128-w-2, y+8);
    display.print(numText);
  }
  display.setTextColor(SH110X_WHITE);
}

void drawStats(int x, int y){
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

void ExitStats(){

}

void statSelection(){
  if(!bAgeSel && !bMassSel && !bStatBackSel){
    if(bUp_FE){
      if((nStatSel & 0x30) >= 0x10)
        nStatSel -= 0x10;
      else
        nStatSel += 0x20;

    }else if(bDown_FE){
      if((nStatSel & 0x20) < 0x20)
        nStatSel += 0x10;
      else
        nStatSel -= 0x20;
    }
  }else if(bAgeSel){
    if(bUp_FE)
      nAgeDisplay ++;
    else if(bDown_FE)
      nAgeDisplay --;
  }else if(bMassSel){
    if(bUp_FE)
      nMassDisplay ++;
    else if(bDown_FE)
      nMassDisplay --;
  }
  if(bCenter_RE)
    nStatSel = nStatSel ^ 1;

  bAgeFoc = (nStatSel==0x00);
  bAgeSel = (nStatSel==0x01);
  bMassFoc = (nStatSel==0x10);
  bMassSel = (nStatSel==0x11);
  bStatBackFoc = (nStatSel==0x20);
  bStatBackSel = (nStatSel==0x21);

  if(bStatBackSel){
    tcxLog.setAge(nAgeDisplay);
    tcxLog.setMass(nMassDisplay);
    nStatSel = 0;
    u16_state = 1;
    bStatBackSel = bStatBackFoc =false;
  }
}


/**
 * Draws the date time 
 * @param someTime the date time to display
 * @param x top left x coordinate of the date time
 * @param y top left y coordinate of the date time
 */
void drawDateTime(DateTime someTime, int x, int y) {
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

/**
 * Used to draw the menu cluster for when no logging is occuring
 * @param x x coordinate of the center of the cluster
 * @param y y coordinate of the center of the cluster
 */
void drawMenuStopped(int x, int y) {
  display.drawBitmap(x - 32, y - 8, epd_bitmap_gear , 16, 16, 1);
  display.drawBitmap(x - 8, y - 8, epd_bitmap_play, 16, 16, 1);
  display.drawBitmap(x + 16, y -8, epd_bitmap_power, 16, 16, 1);

}

/**
 * Used to draw the menu cluster for when logging is running
 * @param x x coordinate of the center of the cluster
 * @param y y coordinate of the center of the cluster
 */
void drawMenuRunning(int x, int y) {
  //display.drawBitmap( x-8, y-26, epd_bitmap_UP,16, 16, 1);
  //display.drawBitmap( x-32, y-8, epd_bitmap_loop,16, 16, 1);
  if (log_data.logging()) {
    display.drawBitmap(x - 8, y - 8, epd_bitmap_pause, 16, 16, 1);
  } else {
    display.drawBitmap(x - 8, y - 8, epd_bitmap_play, 16, 16, 1);
  }
  display.drawBitmap(x + 16, y - 8, epd_bitmap_stop, 16, 16, 1);
}

//screens
void GUI() {

  switch (u16_state) {
    case 0:  //main screen
      if (nDeviceScanDelay < (nCurrentTimeMillis - nLastScan)) {
        if (!BT_Device::all_devices_discovered() && !Bluefruit.Scanner.isRunning()) {
          Bluefruit.Scanner.setRxCallback(scan_callback);
          Bluefruit.Scanner.filterUuid(GATT_CSC_UUID, UUID16_SVC_HEART_RATE, GATT_CPS_UUID, GATT_BAT_UUID);
          Bluefruit.Scanner.useActiveScan(true);
          Bluefruit.Scanner.start(0);
        }
        nLastScan = nCurrentTimeMillis;
      }

      bSysOff = bRight_long && !b_Running && !bSwitchOnDelay;

      drawMain();

      if (b_Running) {
        if (bRight_RE && b_Running) {
          b_Running = false;
        } else if (bCenter_short) {
          log_data.playPause_logging();
        }
      } else if (!b_Running) {
        if (bCenter_short) {
          b_Running = true;

        } else if(bLeft_RE){
          u16_state = 1;
          if(Bluefruit.Scanner.isRunning())
            Bluefruit.Scanner.stop();
        }
      }

      if (b_Running && !b_Running_Prev) {
        if(Bluefruit.Scanner.isRunning())
          Bluefruit.Scanner.stop();
        Serial.println("start logger");
        log_data.start_logging(nCurrentTime);
        log_data.play_logging();
        tcxLog.startLogging(nCurrentTime);
        uBlox_Cont();
      } else if (!b_Running && b_Running_Prev) {
        log_data.pause_logging();
        log_data.write_tail(f32_avgSpeed, f32_max_speed, f32_avgCad, f32_max_cad);
        // uBlox_Idle();
        bWriteTCX = true;
      }
      b_Running_Prev = b_Running;

      break;

    case 1:  //settings menu
      drawSettings();
      settingsSelect();
      break;

    case 2:  //time

      dtTimeDisplay = updateTime(dtTimeDisplay);
      drawDateTime(dtTimeDisplay, 0, 0);
      timeSelection();
      ExitTime();
      break;

    case 3:  //human info
      drawStats(0,0);
      statSelection();
      break;

    case 4:  //nearby devices
      if (bStateEntry && !Bluefruit.Scanner.isRunning()) {
        Bluefruit.Scanner.setRxCallback(scan_discovery);
        Bluefruit.Scanner.filterUuid(GATT_CSC_UUID, UUID16_SVC_HEART_RATE, GATT_CPS_UUID);
        Bluefruit.Scanner.useActiveScan(1);
        Bluefruit.Scanner.start(0);  // // 0 = Don't stop scanning after n seconds
      }
      showDevices();
      deviceSelection();
      ExitDevices();
      break;

    case 5:   //gps settings
      drawGPSsettings();
      GPSSelect();
  }
  stateTransition();
  bStateEntry = u16_state_prev != u16_state;
  u16_state_prev = u16_state;
}

bool GPS_Param_Sel;
void GPSSelect(){
  int numSettings = std::end(PM2_settings)-std::begin(PM2_settings);
  if(!GPS_Param_Sel){
    if (bUp_RE) {
      if (s16GPS_SettingsSel >= 1)
      {
        s16GPS_SettingsSel --;
      }else{
        s16GPS_SettingsSel = numSettings - 1;
      }
    }
    if (bDown_RE) {
      if (s16GPS_SettingsSel < (numSettings-1)){
        s16GPS_SettingsSel ++;
      }else{
        s16GPS_SettingsSel = 0;
      }
    }
    if (bLeft_RE) {
      u16_state = 1;
      uBlox_PM2();
      uBlox_Save();
    }
  }else{
    if(bUp_RE)
      PM2_settings[s16GPS_SettingsSel].val++;
    else if(bUp && bUp_short_held)
      PM2_settings[s16GPS_SettingsSel].val++;
    else if(bUp_long)
      PM2_settings[s16GPS_SettingsSel].val+=10*nUp_long_mult;

    if(bDown_RE)
      PM2_settings[s16GPS_SettingsSel].val--;
    else if(bDown && bDown_short_held)
      PM2_settings[s16GPS_SettingsSel].val--;
    else if(bDown_long)
      PM2_settings[s16GPS_SettingsSel].val-=10*nDown_long_mult;

    if(PM2_settings[s16GPS_SettingsSel].val>PM2_settings[s16GPS_SettingsSel].maxVal)
      PM2_settings[s16GPS_SettingsSel].val=PM2_settings[s16GPS_SettingsSel].maxVal;

    if(PM2_settings[s16GPS_SettingsSel].val<PM2_settings[s16GPS_SettingsSel].minVal)
      PM2_settings[s16GPS_SettingsSel].val=PM2_settings[s16GPS_SettingsSel].minVal;
  }
  if (bCenter_RE) {
    GPS_Param_Sel = !GPS_Param_Sel;
  }
}

void drawGPSsettings(){

  display.setTextColor(SH110X_WHITE);
  int numSettings = std::end(PM2_settings)-std::begin(PM2_settings);

  int16_t index1 = s16GPS_SettingsSel-2;
  if(index1 < 0)
    index1 = numSettings+index1;

  int16_t index2 = s16GPS_SettingsSel-1;
  if(index2 < 0)
    index2 = numSettings+index2;

  int16_t index4 = s16GPS_SettingsSel+1;
  if(index4 >(numSettings-1))
    index4 = index4-numSettings;

  int16_t index5 = s16GPS_SettingsSel+2;
  if(index5 >(numSettings-1))
    index5 = index5-numSettings;
  
  drawSelectable(0,0,PM2_settings[index1].name,PM2_settings[index1].val,false,false);
  drawSelectable(0,20,PM2_settings[index2].name,PM2_settings[index2].val,false,false);
  drawSelectable(0,40,PM2_settings[s16GPS_SettingsSel].name,PM2_settings[s16GPS_SettingsSel].val,true,GPS_Param_Sel);
  drawSelectable(0,60,PM2_settings[index4].name,PM2_settings[index4].val,false,false);
  drawSelectable(0,80,PM2_settings[index5].name,PM2_settings[index5].val,false,false);

}

void settingsSelect(){
  if (bUp_RE) {
    if (s16SettingsSel >= 1)
    {
      s16SettingsSel --;
    }else{
      s16SettingsSel = s16NumSettings - 1;
    }
  }
  if (bDown_RE) {
    if (s16SettingsSel < (s16NumSettings-1)){
      s16SettingsSel ++;
    }else{
      s16SettingsSel = 0;
    }
  }
  if (bCenter_RE) {
    switch (s16SettingsSel) {
      case 0:
        u16_state = 2;
        dtTimeDisplay = rtc.now();
        break;
      case 1:
        u16_state = 4;
        break;
      case 2:
        nAgeDisplay = tcxLog.getAge();
        nMassDisplay = tcxLog.getMass();
        bCenter_FE=false;
        bAgeSel=false;
        bMassSel=false;
        bAgeFoc=true;
        bMassFoc=false;
        u16_state = 3;
        break;
      case 3:
        u16_state = 0;
        s16SettingsSel = 0;
        break;
      case 4:
        u16_state = 5;
        break;
    }
    saveJson();
    bCenter_Seen = false;
  } 
}

void drawSettings(){

  display.setTextColor(SH110X_WHITE);

  int16_t aboveIndex = s16SettingsSel-1;
  if(s16SettingsSel == 0)
    aboveIndex = s16NumSettings-1;

  int16_t belowIndex = s16SettingsSel+1;
  if(s16SettingsSel == (s16NumSettings-1))
    belowIndex = 0;

  drawSettingsmenuItem(0, 13, false, aboveIndex);
  drawSettingsmenuItem(0, 47, true, s16SettingsSel);
  drawSettingsmenuItem(0, 81, false, belowIndex);

}

struct SettingsItem {
    const unsigned char* img;
    int x;
    int y;
    char name[32];
};

SettingsItem SettingsItems[s16NumSettings] = {
  {epd_bitmap_clock_large,32,32,"Time"},
  {epd_bitmap_bluetooth_large,32,32,"BLE"},
  {epd_bitmap_heart_large,32,32,"Info"},
  {epd_bitmap_left_arrow_large,32,32,"Back"},
  {epd_bitmap_antenna_large,32,32,"GPS"}
};

void drawSettingsmenuItem(int x, int y, bool focus, int16_t menuIndex) {
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(x, y);
 

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(SettingsItems[menuIndex].name, x, y, &x1, &y1, &w, &h);
  
  display.drawBitmap(x+2, y+1, SettingsItems[menuIndex].img, SettingsItems[menuIndex].x, SettingsItems[menuIndex].y, 1);
  display.setCursor(x+32+8, y+8);
  display.print(SettingsItems[menuIndex].name);
  if (focus)
    display.drawRect(x, y, 127, 34, 1);
}

void deviceSelection() {
  if (bUp_RE) {
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

  if (bDown_RE) {
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
    if (bCenter_RE) {
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
  bBackSelDev = bCenter_RE && bBackFocDev;
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

void ExitDevices() {
  if (bBackSelDev) {
    s16DeviceSel = 0;
    s16DeviceWindowStart = 0;
    u16_state = 1;
    Bluefruit.Scanner.stop();

    bBackSelDev = false;
    bBackFocDev = false;
  }
}

void saveJson(){

  JsonDocument doc;
  JsonArray devices = doc["devices"].to<JsonArray>();
    int j = 0;

  for (int i = 0; i < nearby_devices.size(); i++) {
    if (nearby_devices[i].stored) {
      devices[j]["name"] = nearby_devices[i].name;
      devices[j]["type"] = nearby_devices[i].type;
      JsonArray device_MAC = devices[j]["MAC"].to<JsonArray>();
      ;
      device_MAC.add(nearby_devices[i].MAC[0]);
      device_MAC.add(nearby_devices[i].MAC[1]);
      device_MAC.add(nearby_devices[i].MAC[2]);
      device_MAC.add(nearby_devices[i].MAC[3]);
      device_MAC.add(nearby_devices[i].MAC[4]);
      device_MAC.add(nearby_devices[i].MAC[5]);
      j++;
    }
  }

  doc["age"] = tcxLog.getAge();
  doc["mass"] = tcxLog.getMass();
    
  JsonArray GPS = doc["GPS"].to<JsonArray>();
  
  int arrayEnd = std::end(PM2_settings)-std::begin(PM2_settings);
  for (int i=0; i<arrayEnd; i++)
  {
    GPS[i]["name"] = PM2_settings[i].name;
    GPS[i]["val"] = PM2_settings[i].val;
  }

  if (SD.exists("/devices.txt"))
    SD.remove("/devices.txt");

  File32 dataFile = SD.open("/devices.txt", FILE_WRITE);

  serializeJson(doc, dataFile);
  Serial.println("");
  dataFile.close();
}

/**
 * Callback invoked when scanner pick up an advertising data
 * @param report Structural advertising data
 */
void scan_discovery(ble_gap_evt_adv_report_t* report) {
  prph_info_t newDevice;
  //get the MAC
  copyMAC(newDevice.MAC, report->peer_addr.addr);
  memset(&newDevice.name,0,32);

  //Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, (uint8_t*)newDevice.name, sizeof(newDevice.name));
  Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, (uint8_t*)newDevice.name, sizeof(newDevice.name));

  bool bMatch = 0;
  //check if the list contains anyhting
  if (nearby_devices.size() > 0) {
    //iterate the list
    for (int i = 0; i < nearby_devices.size(); i++) {
      //check if the new found device matches any of the devices in the list
      if (compareMAC(nearby_devices[i].MAC, newDevice.MAC)) {
        bMatch = 1;
      }
    }
  }
  //if the new device is unique add it
  if (!bMatch) {
    newDevice.stored=false;
    if(Bluefruit.Scanner.checkReportForUuid(report, GATT_CSC_UUID))
      newDevice.type = E_Type_BT_Device::bt_csc;
    if(Bluefruit.Scanner.checkReportForUuid(report, UUID16_SVC_HEART_RATE))
      newDevice.type = E_Type_BT_Device::bt_hrm;
    if(Bluefruit.Scanner.checkReportForUuid(report, GATT_CPS_UUID))
      newDevice.type = E_Type_BT_Device::bt_cps;

    nearby_devices.push_back(newDevice);
  }

  // For Softdevice v6: after received a report, scanner will be paused
  // We need to call Scanner resume() to continue scanning
  Bluefruit.Scanner.resume();
}

/**
 * Callback invoked when scanner pick up an advertising data
 * @param report Structural advertising data
 */
void scan_callback(ble_gap_evt_adv_report_t* report) {
  Serial.println("Found Device:");
  Serial.print("MAC: ");
  Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
  Serial.print("\n");
  // Since we configure the scanner with filterUuid()
  // Scan callback only invoked for device with csc service advertised
  // Connect to device with csc service in advertising
  
  BT_Device* device = BT_Device::getDeviceWithMAC(report->peer_addr.addr);
  if (device != NULL) {
    Serial.println("Match!");
    copyMAC(toConnectMAC, report->peer_addr.addr);
    Bluefruit.Central.connect(report);
  }
}

void stateTransition() {
}

DateTime updateTime(DateTime someTime) {
  int8_t s8AddVal;
  if (bUp_RE) {
    s8AddVal = 1;
  } else if (bDown_RE) {
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

/**
 * Determines what value in the data time display is focused or slected
 */
void timeSelection() {
  //button inputs
  if (!(bHourSel | bMinuteSel | bSecondSel | bDaySel | bMonthSel | bYearSel)) {
    if (bUp_RE) {
      if ((s16timeSel & 0x0300) >= 0x0100)
        s16timeSel -= 0x0100;
      else
        s16timeSel += 0x0200;
    }

    if (bDown_RE) {
      if ((s16timeSel & 0x0200) < 0x0200)
        s16timeSel += 0x100;
      else
        s16timeSel -= 0x0200;
    }

    if (bLeft_RE) {
      if ((s16timeSel & 0x0030) >= 0x0010)
        s16timeSel -= 0x0010;
      else
        s16timeSel += 0x0020;
    }

    if (bRight_RE) {
      if ((s16timeSel & 0x0020) < 0x0020)
        s16timeSel += 0x0010;
      else
        s16timeSel &= 0xFFCF;
    }
  }

  if (s16timeSel > 0x211) {
    s16timeSel &= 0xFF0F;
  }

  if (bCenter_RE)
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

void ExitTime() {
  if (bSaveSel) {
    rtc.adjust(dtTimeDisplay);
  }
  if (bBackSel | bSaveSel) {
    s16timeSel = 0;
    u16_state = 1;
    bBackFoc = false;
    bBackSel = false;
    bSaveFoc = false;
    bSaveSel = false;
  }
}

//draw the main screen
void drawMain() {
  //draw status symbols
  display.setTextSize(1);
  display.drawBitmap(95, 0, epd_bitmap_battery, 32, 16, 1);
  display.setCursor(102, 5);
  display.print(nBatteryPercentage);
  if(gps.location.isValid()){
    display.drawBitmap(0, 0, epd_bitmap_antenna, 16, 16, 1);
  }
  
  display.setTextSize(4);
  display.setCursor(0, 16);
  if (f32_kph < 10)
    display.print(' ');
  display.print(f32_kph, 1);
  display.setTextSize(1);
  display.setCursor(96, 16);
  display.println("  k/h");

  display.setTextSize(2);
  display.setCursor(47, 48);

  if (f32_cadence < 10)
    display.print("   ");
  else if (f32_cadence < 100)
    display.print("  ");
  else
    display.print(" ");
  display.print(f32_cadence, 0);
  display.setTextSize(1);
  display.setCursor(96, 48);
  display.print("  rpm");

  //display.drawBitmap(60, 62, epd_bitmap_heart, 16, 16, 1);
  display.setTextSize(2);
  display.setCursor(47, 64);
  if(f32_bpm<10)
    display.print("   ");
  else if (f32_bpm < 100)
    display.print("  ");
  else
    display.print(" ");
  display.print(f32_bpm, 0);
  display.setTextSize(1);
  display.setCursor(96, 64);
  display.print("  bpm");

  if (b_Running) {
    drawMenuRunning(64, 88);
    display.setCursor(0, 112);
    display.setTextSize(2);
    display.print(log_data.elapsedString());
  } else {
    drawMenuStopped(64, 88);
  }
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void connect_callback(uint16_t conn_handle) {
  bool restartScan = false;
  
  BT_Device* device = BT_Device::getDeviceWithMAC(toConnectMAC);
  if (device != NULL) {
    if(!device->discovered())
      device->discover(conn_handle);
  }
  if (!BT_Device::all_devices_discovered()) {
    Bluefruit.Scanner.resume();
  }
}

void readIMU() {
  f32_acc_x = myIMU.readFloatAccelX();
  f32_acc_y = myIMU.readFloatAccelY();
  f32_acc_z = myIMU.readFloatAccelZ();
  f32_gyro_x = myIMU.readFloatGyroX();
  f32_gyro_y = myIMU.readFloatGyroY();
  f32_gyro_z = myIMU.readFloatGyroZ();
}
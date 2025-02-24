#include <Arduino.h>

#define GPSSerial Serial1
static const uint32_t GPSBaud = 9600;

uint8_t UBX_CFG_PM2[] = {
  0xB5, 0x62, 0x06, 0x3B, 0x2C, 0x00, 0x01, 0x06, 0x00, 0x00, 0x0E, 0x00, 0x40, 0x01, 
  0x60, 0xEA, 0x00, 0x00, 0x60, 0xEA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
  0x00, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x4F, 0xC1, 0x03, 0x00, 0x86, 0x02, 0x00, 0x00, 
  0xFE, 0x00, 0x00, 0x00, 0x64, 0x40, 0x01, 0x00, 0xC3, 0x58
};

uint8_t UBX_CFG_CFG[] = {
  0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x1D, 0xAB};

uint8_t UBX_CFG_PMS[] = {
  0xB5, 0x62, 0x06, 0x86, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x5A};

uint8_t UBX_CFG_RXM_PSM[] = {
  0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92};

uint8_t UBX_CFG_RXM_CONT[] = {
  0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x00, 0x21, 0x91};

uint8_t UBX_CFG_PWR_STP[] = {
  0xB5, 0x62, 0x06, 0x57, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x4F, 0x54, 0x53, 0xAC, 0x85};

uint8_t UBX_CFG_PWR_BKP[] = {
  0xB5, 0x62, 0x06, 0x57, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x4B, 0x43, 0x42, 0x86, 0x46};

struct param {
  const char name[32];
  uint32_t val;
  const uint32_t maxVal;
  const uint32_t minVal;
};

param PM2_settings[]={
  {"maxStartupStateDur",0,255,0},
  {"extintSel",0,1,0},
  {"extintWake",0,1,0},
  {"extintBackup",0,1,0},
  {"limitPeakCurr",0,1,0},
  {"waitTimeFix",0,1,0},
  {"updateRTC",1,1,0},
  {"updateEPH",1,1,0},
  {"doNotEnterOff",0,1,0},
  {"mode",0,1,0},
  {"updatePeriod",60000,4294967295,0},
  {"searchPeriod",10,4294967295,0},
  {"gridOffset",0,4294967295,0},
  {"onTime",2,65535,0},
  {"minAcqTime",0,65535,0}
};

void checkSum(uint8_t *command, uint8_t length) {
   int CK_A = 0, CK_B = 0;
  for(int i=2;i<=length-3;i++){
    CK_A = CK_A + command[i];
    CK_B = CK_B + CK_A;
  }
  command[length-2] = CK_A;
  command[length-1] = CK_B;
}

void sendUBXCommand(uint8_t *command, uint8_t length) {
  GPSSerial.flush();
  GPSSerial.write((char)0x00);
  delay(50);
  checkSum(command, length);
  for (uint8_t i = 0; i < length; i++) {
    GPSSerial.write(command[i]);
  }
}

void uBlox_PM2_Set_maxStartupStateDur(uint8_t maxStartupStateDur){
  uint8_t* tmp = (uint8_t*)&UBX_CFG_PM2[8];
  *tmp=maxStartupStateDur;  //Maximum time to spend in Acquisition state. If 0: bound disabled
}

void uBlox_PM2_Set_flags(bool extintSel, bool extintWake, bool extintBackup, uint8_t limitPeakCurr, bool waitTimeFix, bool updateRTC, bool updateEPH, bool doNotEnterOff, uint8_t mode){
  uint32_t* tmp = (uint32_t*)&UBX_CFG_PM2[10];

  *tmp = *tmp && extintSel<<4;  //ext int select: 0 = EXTINT0, 1 = EXTINT1
  *tmp = *tmp && extintWake<<5;  //ext int Wake: 0 = disabled, 1 = enabled, keep receiver awake as long as selected EXTINT pin is 'high'
  *tmp = *tmp && extintBackup<<6;  //ext int Backup: 0 = disabled, 1 = enabled, force receiver into BACKUP mode when selected EXTINT pin is 'low'

  *tmp = *tmp && limitPeakCurr<<8;  //limit Peak Curr: 00 = disabled, 01 = enabled, peak current is limited, 10 = reserved, 11 = reserved
  *tmp = *tmp && waitTimeFix<<10; //wait Time Fix: 0 = wait for normal fix OK before starting on time, 1 = wait for time fix OK before starting on time
  *tmp = *tmp && updateRTC<<11; //update RTC: 0 = do not wake up to update RTC. RTC is updated during normal on-time, 1 = update RTC. The receiver adds extra wake-up cycles to update the RTC
  *tmp = *tmp && updateEPH<<12; //update EPH: 0 = do not wake up to update Ephemeris data, 1 = update Ephemeris. The receiver adds extra wake-up cycles to update the Ephemeris data

  *tmp = *tmp && doNotEnterOff<<16; //do Not Enter Off: 0 = receiver enters (Inactive) Awaiting next search state, 1 = receiver does not enter (Inactive) Awaiting next search state but keeps trying to acquire a fix instead
  *tmp = *tmp && mode<<17; //mode: 00 = ON/OFF operation , 01 = cyclic tracking operation, 10 = reserved, 11 = reserved
}

void uBlox_PM2_Set_updatePeriod(uint32_t updatePeriod){
  uint32_t* tmp = (uint32_t*)&UBX_CFG_PM2[14];
  *tmp=updatePeriod;  //Position update period. If set to 0, the receiver will never retry a fix and it will wait for external events
}

void uBlox_PM2_Set_searchPeriod(uint32_t searchPeriod){
  uint32_t* tmp = (uint32_t*)&UBX_CFG_PM2[18];
  *tmp=searchPeriod;  //Acquisition retry period if previously failed. If set to 0, the receiver will never retry a startup
}

void uBlox_PM2_Set_gridOffset(uint32_t gridOffset){
  uint32_t* tmp = (uint32_t*)&UBX_CFG_PM2[22];
  *tmp=gridOffset;  //Grid offset relative to GPS start of week
}

void uBlox_PM2_Set_onTime(uint16_t onTime){
  uint16_t* tmp = (uint16_t*)&UBX_CFG_PM2[26];
  *tmp=onTime;  //Time to stay in Tracking state
}

void uBlox_PM2_Set_minAcqTime(uint16_t minAcqTime){
  uint16_t* tmp = (uint16_t*)&UBX_CFG_PM2[28];
  *tmp=minAcqTime;  //minimal search time
}

void uBlox_PM2(){
  uBlox_PM2_Set_maxStartupStateDur( (uint8_t)PM2_settings[0].val);
  uBlox_PM2_Set_flags(              (bool)   PM2_settings[1].val,
                                    (bool)   PM2_settings[2].val,
                                    (bool)   PM2_settings[3].val,
                                    (uint8_t)PM2_settings[4].val,
                                    (bool)   PM2_settings[5].val,
                                    (bool)   PM2_settings[6].val,
                                    (bool)   PM2_settings[7].val,
                                    (bool)   PM2_settings[8].val,
                                    (uint8_t)PM2_settings[9].val);
  uBlox_PM2_Set_updatePeriod(                PM2_settings[10].val);
  uBlox_PM2_Set_searchPeriod(                PM2_settings[11].val);
  uBlox_PM2_Set_gridOffset(                  PM2_settings[12].val);
  uBlox_PM2_Set_onTime(            (uint16_t)PM2_settings[13].val);
  uBlox_PM2_Set_minAcqTime(        (uint16_t)PM2_settings[14].val);

  sendUBXCommand(UBX_CFG_PM2,sizeof(UBX_CFG_PM2));
}

void uBlox_OFF(){
  sendUBXCommand(UBX_CFG_RXM_PSM,sizeof(UBX_CFG_RXM_PSM));
}

void uBlox_Save(){
  sendUBXCommand(UBX_CFG_CFG,sizeof(UBX_CFG_CFG));
}

void uBlox_Cont(){
  sendUBXCommand(UBX_CFG_RXM_CONT,sizeof(UBX_CFG_RXM_CONT));
}
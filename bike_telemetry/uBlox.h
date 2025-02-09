#include <Arduino.h>

#define GPSSerial Serial1
static const uint32_t GPSBaud = 9600;

uint8_t UBX_RXM_PMREQ[] = {
  0xB5, 0x62, 0x02, 0x41,0x10,//header/class/ID/length
  0x00,                       //version       (unsigened char)  0x0
  0x00, 0x00, 0x00,           //reserved      (U1[3])           0x01
  0x00, 0x00, 0x00, 0x00,     //duration      (U4)              0x04
  0x00, 0x00, 0x00, 0x02,     //flags         (X4)              0x08
  0x00, 0x00, 0x00, 0x08,     //wake sources  (X4)              0x12
  0x00, 0x00                  //Checksum
};

uint8_t UBX_CFG_PMS[] = {
  0xB5, 0x62, 0x06, 0x86, 0x08, 0x00, 0x00, 0x02, 0x1E, 0x00, 0x01, 0x00, 0x00, 0x00, 0xB5, 0x20};

uint8_t UBX_CFG_PM2[] = {
  0xB5, 0x62, 0x06, 0x3B, 0x2C, 0x00, 
  01, 0x06, 0x00, 0x00, 0x0E, 0x91, 0x41, 0x01, 0x30, 0x75, 0x00, 0x00, 0x10,
  0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x4F, 0xC1, 0x03, 0x00,
  0x87, 0x02, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x64, 0x40, 0x01, 0x00, 0xA0, 0x65
};

uint8_t UBX_CFG_PM2_60S[] = {
0xB5, 0x62, 0x06, 0x3B, 0x2C, 0x00, 0x01, 0x06, 0x00, 0x00, 0x0E, 0x81, 0x41, 0x01, 0x60, 0xEA, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x4F, 0xC1, 0x03, 0x00, 0x87, 0x02, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x64, 0x40, 0x01, 0x00, 0x34, 0x9C
};

uint8_t UBX_CFG_PM2_10S[] = {
0xB5, 0x62, 0x06, 0x3B, 0x2C, 0x00, 0x01, 0x06, 0x00, 0x00, 0x0E, 0x80, 0x41, 0x01, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x4F, 0xC1, 0x03, 0x00, 0x87, 0x02, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x64, 0x40, 0x01, 0x00, 0x20, 0x8C
};

uint8_t UBX_CFG_RXM_PSM[] = {
  0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92};

uint8_t UBX_CFG_RXM_CONT[] = {
  0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x00, 0x21, 0x91};

uint8_t UBX_CFG_PWR[] = {
  0xB5, 0x62, 0x06, 0x57, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x4B, 0x43, 0x42, 0x86, 0x46};

uint8_t UBX_CFG_CFG[] = {
  0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x1B, 0xA9};

void checkSum(uint8_t *command, uint8_t length) {
   int CK_A = 0, CK_B = 0;
  for(int i=2;i<length-3;i++){
    CK_A = CK_A + command[i];
    CK_B = CK_B + CK_A;
  }
  command[length-2] = CK_A;
  command[length-1] = CK_B;
}

void sendUBXCommand(uint8_t *command, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    GPSSerial.write(command[i]);
  }
}

void uBlox_Save(){
  checkSum(UBX_CFG_PWR,sizeof(UBX_CFG_PWR));
  sendUBXCommand(UBX_CFG_CFG,sizeof(UBX_CFG_CFG));
}

void uBlox_OFF(){
  GPSSerial.write(0xFF);
  delay(500);
  sendUBXCommand(UBX_CFG_PM2_60S,sizeof(UBX_CFG_PM2_60S));
  delay(50);
  sendUBXCommand(UBX_CFG_RXM_PSM,sizeof(UBX_CFG_RXM_PSM));
  delay(50);
  sendUBXCommand(UBX_CFG_CFG,sizeof(UBX_CFG_CFG));
}

void uBlox_PSM(){
  //checkSum(UBX_CFG_RXM,sizeof(UBX_CFG_RXM));
  sendUBXCommand(UBX_CFG_RXM_PSM,sizeof(UBX_CFG_RXM_PSM));
}

void uBlox_Cont(){
  //checkSum(UBX_CFG_RXM,sizeof(UBX_CFG_RXM));
  GPSSerial.write(0xFF);
  delay(500);
  sendUBXCommand(UBX_CFG_RXM_CONT,sizeof(UBX_CFG_RXM_CONT));
  delay(50);
  sendUBXCommand(UBX_CFG_CFG,sizeof(UBX_CFG_CFG));
}

void uBlox_Idle(){
  GPSSerial.write(0xFF);
  delay(500);
  sendUBXCommand(UBX_CFG_PM2_10S,sizeof(UBX_CFG_PM2_10S));
  delay(50);
  sendUBXCommand(UBX_CFG_RXM_PSM,sizeof(UBX_CFG_RXM_PSM));
  delay(50);
  sendUBXCommand(UBX_CFG_CFG,sizeof(UBX_CFG_CFG));
}

void uBlox_PowerSaveMode_Config(){
  //checkSum(UBX_CFG_PM2,sizeof(UBX_CFG_PM2));
  sendUBXCommand(UBX_CFG_PM2_10S,sizeof(UBX_CFG_PM2_10S));
}
#ifndef utils_H
#define utils_H
#include <Arduino.h>
#include "SdFat.h"
#define SD_CS       D3      // SD CS

static File32 debugLog;

inline bool compareMAC(uint8_t *mac1, uint8_t *mac2)
{
  bool bMatch = 1;
  bMatch &= mac1[0] == mac2[0];
  bMatch &= mac1[1] == mac2[1];
  bMatch &= mac1[2] == mac2[2];
  bMatch &= mac1[3] == mac2[3];
  bMatch &= mac1[4] == mac2[4];
  bMatch &= mac1[5] == mac2[5];
  return bMatch;
}

inline void copyMAC(uint8_t *dest_MAC, uint8_t *src_MAC)
{
  dest_MAC[0] = src_MAC[0];
  dest_MAC[1] = src_MAC[1];
  dest_MAC[2] = src_MAC[2];
  dest_MAC[3] = src_MAC[3];
  dest_MAC[4] = src_MAC[4];
  dest_MAC[5] = src_MAC[5];
}

inline void logInfoln(const char* text)
{
  Serial.println(text);
  if(!debugLog)
    debugLog.open("/log.txt", FILE_WRITE);
  debugLog.println(text);
}

inline void logInfo(const char* text)
{
  Serial.print(text);
  if(!debugLog)
    debugLog.open("/log.txt", FILE_WRITE);
  debugLog.println(text);
}

inline void logInfo(String text)
{
  char* buff;
  text.toCharArray(buff, text.length());
  logInfo(buff);
}

inline void logInfoln(String text)
{
  char* buff;
  text.toCharArray(buff, text.length());
  logInfoln(buff);
}

#endif /* utils_H */
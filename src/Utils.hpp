#ifndef utils_H
#define utils_H
#include <Arduino.h>
#include <SdFat.h>
#define SD_CS       D3      // SD CS

static File32 debugLog;

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
#ifndef SDCARD_H
#define SDCARD_H

#include <Arduino.h>
#include <SdFat.h>
#include <RTClib.h>
#include <memory>

#include "StorageInterface.hpp"
#define SD_CS D3

class SDCardSystem : public IStorage {
public:
    bool init(RTC_DS3231* rtc);

    // Generic file accessors
    File32 openFile(const char* path, oflag_t mode) override;
    bool remove(const char* path) override;
    bool exists(const char* path) override;

    bool isMounted() const { return mounted; }
    bool isBusy() const { return busy; }
    void unMount() {
        sd.end();
        mounted = false;
    }
    

private:
    SdFat32 sd;              // the only filesystem instance
    bool mounted = false;
    mutable bool busy = false;
    static RTC_DS3231* _rtc;


    static void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) {
        DateTime now = _rtc->now();

        // Return date using FS_DATE macro to format fields.
        *date = FS_DATE(now.year(), now.month(), now.day());

        // Return time using FS_TIME macro to format fields.
        *time = FS_TIME(now.hour(), now.minute(), now.second());

        // Return low time bits in units of 10 ms, 0 <= ms10 <= 199.
        *ms10 = now.second() & 1 ? 100 : 0;
    }
};

#endif /* SDCARD_H */
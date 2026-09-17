#ifndef SDCARD_H
#define SDCARD_H

#include <Arduino.h>
#include <SdFat.h>
#include <memory>

#include "StorageInterface.hpp"
#include "DataModel/TimeDataProvider.hpp"

#define SD_CS D1

class SDCardSystem : public IStorage {
public:
    bool init(timeData* date);

    // Generic file accessors
    File32 openFile(const char* path, oflag_t mode) override;
    bool remove(const char* path) override;
    bool exists(const char* path) override;
    bool listFiles(const char* dir, StorageEntry* out, uint8_t max, uint8_t& count) override;

    bool isMounted() const { return mounted; }

    // Discard all cached filesystem state and re-read it from the card.
    // Mandatory after the host has written to the card via USB MSC: SdFat
    // trusts its caches, so stale cached FAT/directory sectors would be
    // traversed against the card's real (new) contents and crash the MCU.
    // All firmware file handles must be closed before calling this.
    bool remount() {
        if (!mounted) return false;
        sd.end();
        mounted = false;
        return init(_date);
    }
    bool isBusy() const { return busy; }
    void unMount() {
        sd.end();
        mounted = false;
    }

    // Block-device access for USB MSC. Returns the underlying SdCard so
    // callers can do raw sector reads/writes without going through the
    // filesystem. Valid only while mounted.
    SdCard* card() { return mounted ? sd.card() : nullptr; }

private:
    SdFat32 sd;              // the only filesystem instance
    bool mounted = false;
    mutable bool busy = false;
    static timeData* _date;

    static void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) {
        
        auto lt = _date->local();

        // Return date using FS_DATE macro to format fields.
        *date = FS_DATE(lt.year, lt.month, lt.day);

        // Return time using FS_TIME macro to format fields.
        *time = FS_TIME(lt.hour, lt.minute, lt.second);

        // Return low time bits in units of 10 ms, 0 <= ms10 <= 199.
        *ms10 = lt.second & 1 ? 100 : 0;
    }
};

#endif /* SDCARD_H */
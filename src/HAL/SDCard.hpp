#ifndef SDCARD_H
#define SDCARD_H

#include <Arduino.h>
#include <SdFat.h>
#include <memory>

#include "StorageInterface.hpp"
#define SD_CS D3

class SDCardSystem : public IStorage {
public:
    bool init();

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
};

#endif /* SDCARD_H */
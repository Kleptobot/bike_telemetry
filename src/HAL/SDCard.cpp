#include "SDCard.hpp"

bool SDCardSystem::init() {
    if (mounted) return true;

    if (!sd.begin(SD_CS)) {
        Serial.println("[SDCardSystem] init failed");
        return false;
    }
    mounted = true;
    Serial.println("[SDCardSystem] mounted (SdFs)");
    return true;
}

File32 SDCardSystem::openFile(const char* path, oflag_t mode) {
    // if (!mounted || busy) return;
    busy = true;

    File32 file;
    if (!file.open(path, mode)) {
        Serial.printf("[SDCardSystem] open failed: %s\n", path);
        busy = false;
        //return;
    }
    return file;
}

bool SDCardSystem::exists(const char* path) {
    return sd.exists(path);
}

bool SDCardSystem::remove(const char* path) {
    if (!mounted || busy) return false;
    return sd.remove(path);
}

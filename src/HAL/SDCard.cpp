#include "SDCard.hpp"

timeData* SDCardSystem::_date;

bool SDCardSystem::init(timeData* date) {
    _date = date;
    if (mounted) return true;

    if (!sd.begin(SD_CS)) {
        Serial.println("[SDCardSystem] init failed");
        return false;
    }
    mounted = true;
    FsDateTime::setCallback(SDCardSystem::dateTime);
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
    if (!mounted) return false;
    return sd.remove(path);
}

bool SDCardSystem::listFiles(const char* dir, StorageEntry* out, uint8_t max, uint8_t& count) {
    count = 0;
    if (!mounted || !out) return false;

    File32 d;
    if (!d.open(dir ? dir : "/", O_RDONLY) || !d.isDirectory()) return false;

    File32 f;
    while (count < max && f.openNext(&d, O_RDONLY)) {
        f.getName(out[count].name, sizeof(out[count].name) - 1);
        out[count].name[sizeof(out[count].name) - 1] = '\0';
        out[count].isDir = f.isDirectory();
        count++;
        f.close();
    }
    d.close();
    return true;
}




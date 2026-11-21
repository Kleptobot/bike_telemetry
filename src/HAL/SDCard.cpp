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

bool SDCardSystem::listFiles(const char* dir, StorageEntry* out, uint8_t max,
                             uint8_t& outCount) {
    outCount = 0;
    if (!mounted || !out || max == 0) return false;

    File32 dirFile;
    if (!dirFile.open(dir, O_RDONLY)) {
        Serial.printf("[SDCardSystem] listFiles: cannot open %s\n", dir);
        return false;
    }
    if (!dirFile.isDir()) {
        Serial.printf("[SDCardSystem] listFiles: %s is not a directory\n", dir);
        dirFile.close();
        return false;
    }

    File32 file;
    while (outCount < max && file.openNext(&dirFile, O_RDONLY)) {
        // Optional: skip hidden/system files (like macOS ._ files or System Volume Information)
        if (file.isHidden() || file.isSystem()) {
            file.close();
            continue;
        }

        char name[64] = {0};
        // getName() automatically gets the Long File Name (LFN) if present,
        // or falls back to the 8.3 short name.
        if (!file.getName(name, sizeof(name))) {
            file.close();
            continue;
        }

        // Skip "." and ".."
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            file.close();
            continue;
        }

        strncpy(out[outCount].name, name, sizeof(out[outCount].name) - 1);
        out[outCount].name[sizeof(out[outCount].name) - 1] = '\0';
        out[outCount].isDir = file.isDir();
        out[outCount].size = static_cast<uint32_t>(file.fileSize());
        outCount++;

        file.close(); // Must close on EVERY iteration
    }

    dirFile.close();
    return true;
}

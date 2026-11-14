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
    // busy spans only the open() call itself: FatFile::open does directory
    // traversal and (for O_CREAT) writes, which is the window isBusy() exists
    // to describe. It used to latch true forever after the first successful
    // open -- no caller consumes it today, but the flag lied, and any future
    // MSC/gate check built on it would wedge permanently after one open.
    busy = true;

    File32 file;
    if (!mounted) {
        Serial.printf("[SDCardSystem] open skipped (unmounted): %s\n", path);
        busy = false;
        return file;
    }
    if (!file.open(path, mode)) {
        Serial.printf("[SDCardSystem] open failed: %s\n", path);
    }
    busy = false;
    return file;
}

bool SDCardSystem::exists(const char* path) {
    return sd.exists(path);
}

bool SDCardSystem::remove(const char* path) {
    if (!mounted) return false;

    // If the file is already gone, nothing to do.
    if (!sd.exists(path)) {
        Serial.printf("[SDCardSystem] remove(%s): already absent\n", path);
        return true;
    }

    // Volume-level remove: opens the file internally with O_WRONLY and removes
    // it. If that internal open fails (stale directory cache, SdFat still
    // considers the file in use, read-only attribute), sd.remove() returns
    // false with no error code set -- the silent failure seen on some logs.
    //
    // NOTE: SdFat caches the volume FAT and directory sectors. If the card
    // was written to behind the cache (e.g. host edited files over USB MSC
    // while a ride was logged, then the card was re-inserted), sd.remove()
    // may traverse stale cache and silently return false while
    // cardErr/sdErr stay 0x00. Flush + remount before the fallback.
    if (sd.remove(path)) {
        Serial.printf("[SDCardSystem] remove(%s): ok (volume-level)\n", path);
        return true;
    }
    // Diagnose why the volume-level remove failed.
    uint32_t cardErr = sd.card() ? sd.card()->errorCode() : 0;
    bool stillExists = sd.exists(path);
    Serial.printf("[SDCardSystem] remove failed: cardErr=0x%02X sdErr=0x%02X exists=%d -- trying fallback\n",
                  path, cardErr, sd.sdErrorCode(), stillExists ? 1 : 0);


    // Fallback: open the file ourselves with O_WRONLY and remove via the open
    // handle. This bypasses sd.remove()'s internal open. If we can't open it
    // for write either, the file is read-only or SdFat has it wedged.
    File32 file;
    if (!file.open(path, O_WRONLY)) {
        Serial.printf("[SDCardSystem] remove(%s): cannot open for write (read-only or wedged)\n", path);
        if (!remount()) {
            Serial.println("[SDCardSystem] remove: remount failed, card unavailable");
            return false;
        }
        if (!file.open(path, O_WRONLY)) {
            Serial.printf("[SDCardSystem] remove(%s): still cannot open for write after remount\n", path);
            return false;
        }
    }

    // Granular diagnostics: SFN remove() checks isWritable() && !isLFN(),
    // then freeChain(), then marks the directory entry deleted and calls
    // cacheSync(). Log each precondition so we can see exactly where it fails.
    Serial.printf("[SDCardSystem] remove(%s) fallback: isWritable=%d isLFN=%d "
                  "dirIndex=%u firstCluster=%lu fileSize=%lu writeErr=%d\n",
                  path, (int)file.isWritable(), (int)file.isLFN(),
                  (unsigned)file.dirIndex(), (unsigned long)file.firstCluster(),
                  (unsigned long)file.fileSize(), (int)file.getWriteError());

    // Try sync() first to flush this file's own cache; then remove().
    file.sync();

    if (file.remove()) {
        Serial.printf("[SDCardSystem] remove(%s): ok (file-handle fallback)\n", path);
        file.close();
        return true;
    }

    Serial.printf("[SDCardSystem] remove(%s): file.remove() failed: cardErr=0x%02X sdErr=0x%02X\n",
                  path, sd.card() ? sd.card()->errorCode() : 0, sd.sdErrorCode());
    file.close();

    // Last resort: remount and retry the volume-level remove once.
    if (!remount()) {
        Serial.println("[SDCardSystem] remove: remount failed, card unavailable");
        return false;
    }
    bool ok = sd.remove(path);
    if (!ok)
        Serial.printf("[SDCardSystem] remove(%s): final retry failed: cardErr=0x%02X sdErr=0x%02X\n",
                      path, sd.card() ? sd.card()->errorCode() : 0, sd.sdErrorCode());
    return ok;
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




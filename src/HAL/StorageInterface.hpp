#ifndef STORAGEINTERFACE_H
#define STORAGEINTERFACE_H

#include <SdFat.h>
#include <cstring>

// A single directory entry returned by IStorage::listFiles. name is a fixed
// buffer (not a String) so listFiles can run without heap allocation. The
// name is the bare filename (no leading "/"), e.g. "20260101_120000.fit".
struct StorageEntry {
    char name[64] = {0};
    bool isDir = false;
    uint32_t size = 0;
};

class IStorage {
public:
    virtual File32 openFile(const char* path, oflag_t mode) = 0;
    virtual bool exists(const char* path) = 0;
    virtual bool remove(const char* path) = 0;
    // List up to `max` entries in `dir`, filling `out`. outCount is set to the
    // number of entries written. Returns false on I/O error or if the card is
    // not mounted. "." and ".." are never reported.
    virtual bool listFiles(const char* dir, StorageEntry* out, uint8_t max,
                           uint8_t& outCount) = 0;
    virtual ~IStorage() = default;
};

#endif /* STORAGEINTERFACE_H */
#ifndef STORAGEINTERFACE_H
#define STORAGEINTERFACE_H

#include <SdFat.h>

// One directory entry returned by IStorage::listFiles.
struct StorageEntry {
    char  name[64];
    bool  isDir;
};

class IStorage {
public:
    virtual File32 openFile(const char* path, oflag_t mode) = 0;
    virtual bool exists(const char* path) = 0;
    virtual bool remove(const char* path) = 0;
    // List up to max entries of a directory into out[], filling count with
    // how many were written. Returns false if the directory could not be
    // opened. Entries are returned in filesystem order (unsorted).
    virtual bool listFiles(const char* dir, StorageEntry* out, uint8_t max, uint8_t& count) = 0;
    virtual ~IStorage() = default;
};

#endif /* STORAGEINTERFACE_H */
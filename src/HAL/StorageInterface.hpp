#pragma once
#include <SdFat.h>

class IStorage {
public:
    virtual File32 openFile(const char* path, oflag_t mode) = 0;
    virtual bool exists(const char* path) = 0;
    virtual bool remove(const char* path) = 0;
    virtual ~IStorage() = default;
};

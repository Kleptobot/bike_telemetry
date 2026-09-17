#ifndef ISETTINGS_H
#define ISETTINGS_H

#include "HAL/StorageInterface.hpp"

// Base interface for all persisted settings domains.
// Each concrete settings class wraps a DataProvider and implements
// load() / save() with only the data-mapping logic -- file I/O
// boilerplate is handled by the JsonFile helper.
class ISettings {
public:
    virtual void load(IStorage* storage) = 0;
    virtual void save(IStorage* storage) = 0;
    virtual ~ISettings() = default;
};

#endif /* ISETTINGS_H */

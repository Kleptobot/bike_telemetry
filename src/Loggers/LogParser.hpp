#pragma once

#include "HAL/StorageInterface.hpp"
#include "Loggers/LogSummary.hpp"

// Common incremental interface used by the log-summary screen. Format-specific
// parsers keep their decoding state and implementation in separate modules.
class LogParser {
public:
    explicit LogParser(IStorage* storage = nullptr) : _storage(storage) {}
    virtual ~LogParser() = default;

    virtual bool open(IStorage* storage, const char* path) = 0;
    virtual bool step() = 0;
    virtual bool done() const = 0;

    const RideStats& stats() const { return _stats; }
    virtual void close() {}

protected:
    IStorage* _storage;
    RideStats _stats;
};

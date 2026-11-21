#ifndef LOGPARSER_HPP
#define LOGPARSER_HPP

#pragma once
#pragma once

#include "HAL/StorageInterface.hpp"
#include "Loggers/LogSummary.hpp"

// Abstract incremental log parser. A parser is opened on a saved ride file,
// then step() is called repeatedly (bounded work per call) until done(). The
// accumulated stats are available via stats().
//
// Three concrete parsers (FIT, TCX, CSV) are provided as thin stubs here: they
// report the file as readable but return empty stats. Real format decoding can
// be wired in later without changing the UI contract.
class LogParser {
public:
    explicit LogParser(IStorage* storage = nullptr) : _storage(storage) {}
    virtual ~LogParser() = default;

    // Open `path` for reading on `storage`. Returns true on success.
    virtual bool open(IStorage* storage, const char* path) = 0;

    // Do a bounded increment of parsing. Returns true while still parsing,
    // false when complete (or on error).
    virtual bool step() = 0;

    // True once parsing has finished.
    virtual bool done() const = 0;

    const RideStats& stats() const { return _stats; }

    // Close the underlying file handle.
    virtual void close() {}

protected:
    IStorage* _storage;
    RideStats _stats;
};

// ---- Stubs ----------------------------------------------------------------
// Each stub opens the file (proving it exists/readable), then immediately
// reports done with empty stats. Replace with real decoders as needed.

class FitLogParser : public LogParser {
public:
    using LogParser::LogParser;
    bool open(IStorage* storage, const char* path) override {
        if (!storage) return false;
        File32 f = storage->openFile(path, O_READ);
        bool ok = f.isOpen();
        if (ok) f.close();
        return ok;
    }
    bool step() override { return false; }
    bool done() const override { return true; }
};

class TcxLogParser : public LogParser {
public:
    using LogParser::LogParser;
    bool open(IStorage* storage, const char* path) override {
        if (!storage) return false;
        File32 f = storage->openFile(path, O_READ);
        bool ok = f.isOpen();
        if (ok) f.close();
        return ok;
    }
    bool step() override { return false; }
    bool done() const override { return true; }
};

class CsvLogParser : public LogParser {
public:
    using LogParser::LogParser;
    bool open(IStorage* storage, const char* path) override {
        if (!storage) return false;
        File32 f = storage->openFile(path, O_READ);
        bool ok = f.isOpen();
        if (ok) f.close();
        return ok;
    }
    bool step() override { return false; }
    bool done() const override { return true; }
};

#endif /* LOGPARSER_HPP */

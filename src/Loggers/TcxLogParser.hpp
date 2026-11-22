#pragma once

#include "Loggers/LogParser.hpp"
#include <stdint.h>
#include "yxml.h"

class TcxLogParser : public LogParser {
public:
    using LogParser::LogParser;
    bool open(IStorage* storage, const char* path) override;
    bool step() override;
    bool done() const override { return _done; }
    void close() override;

private:
    // LibYxml streaming parser: yxml_parse() consumes one char at a time.
    // No whole-file buffer is allocated; only a fixed read chunk is used.
    yxml_t _yxml;
    unsigned char _yxmlStack[512];  // yxml element/attribute name stack

    // File streaming
    File32 _file;
    char _readBuf[256];
    size_t _readPos = 0;
    size_t _readLen = 0;
    bool _eofReached = false;

    bool _done = false;

    // Element name stack for context tracking (TCX nesting is shallow;
    // 12 levels is more than enough for TrainingCenterDatabase→Lap→Trackpoint).
    char _elemNames[12][48];
    int8_t _elemDepth = 0;

    // Accumulated character content for the element currently being parsed.
    String _currentText;

    // Running totals for trackpoint-level samples (averages computed at end).
    uint32_t _totalHr = 0, _hrCount = 0;
    uint32_t _totalPower = 0, _powerCount = 0;
    uint32_t _totalCadence = 0, _cadenceCount = 0;

    bool _lapHasDuration = false;
    bool _lapHasTrackpointTime = false;
    long long _lapFirstTrackpointTime = 0;
    long long _lapLastTrackpointTime = 0;

    void _pushElem(const char* name);
    void _popElem();
    const char* _elemParent() const;
    void _handleToken(yxml_ret_t r);
    void _processElemEnd(const char* elemName);
    void _finalise();
    bool _parseTimestamp(const String& text, long long& timestamp) const;
};

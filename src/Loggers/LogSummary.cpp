#include "LogSummary.hpp"
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// Days since 1970-01-01 (proleptic Gregorian). Compact civil-algorithms form
// so the parser needs no library date handling.
// ---------------------------------------------------------------------------
static int daysSinceEpoch(int y, int m, int d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int)doe - 719468;
}

// Parse the ISO prefix "YYYY-MM-DDTHH:MM:SS" the CSV time column uses
// (trailing 'Z' / offset is ignored; consistent within a single ride).
static long long isoToEpoch(const char* s) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, se = 0;
    if (sscanf(s, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &se) != 6) return -1;
    return (long long)daysSinceEpoch(y, mo, d) * 86400LL
         + (long long)h * 3600LL + (long long)mi * 60LL + se;
}

CsvLogParser::CsvLogParser() { reset(); }

void CsvLogParser::reset() {
    _done         = false;
    _headerSkipped = false;
    _lineLen      = 0;
    _haveTime     = false;
    _firstEpoch = _lastEpoch = _prevEpoch = 0;
    _prevSpeed    = 0.0f;
    _sumHr = _sumPwr = _sumCad = 0.0f;
    _maxCad       = 0.0f;
    _hrN = _pwrN = _cadN = 0;
    _stats = RideStats{};
    if (_file.isOpen()) _file.close();
}

bool CsvLogParser::open(IStorage* storage, const char* path) {
    reset();
    if (!storage) return false;
    _file = storage->openFile(path, FILE_READ);
    return (bool)_file;
}

void CsvLogParser::close() {
    if (_file.isOpen()) _file.close();
    _done = true;
}

bool CsvLogParser::step() {
    char chunk[96];
    int n = _file.read(chunk, sizeof(chunk));
    for (int i = 0; i < n; i++) {
        const char c = chunk[i];
        if (c == '\n') {
            if (_lineLen > 0) { _buf[_lineLen] = 0; parseLine(_buf); _lineLen = 0; }
        } else if (c != '\r') {
            if (_lineLen < sizeof(_buf) - 1) _buf[_lineLen++] = c;
        }
    }

    if (n <= 0) {   // EOF (or error) — parse any trailing partial line, finalize.
        if (_lineLen > 0) { _buf[_lineLen] = 0; parseLine(_buf); _lineLen = 0; }
        _done = true;
        _stats.durationSec = (_haveTime && _lastEpoch >= _firstEpoch && _firstEpoch > 0)
                           ? (uint32_t)(_lastEpoch - _firstEpoch) : 0;
        _stats.avgHr       = _hrN  ? _sumHr  / (float)_hrN  : 0.0f;
        _stats.avgPower    = _pwrN ? _sumPwr / (float)_pwrN : 0.0f;
        _stats.avgCadence  = _cadN ? _sumCad / (float)_cadN : 0.0f;
        _stats.avgSpeedKmh = _stats.durationSec > 0
                           ? _stats.distanceM / (float)_stats.durationSec * 3.6f : 0.0f;
        _stats.valid = true;
    }
    return n > 0;
}

void CsvLogParser::parseLine(const char* line) {
    if (!_headerSkipped) {
        _headerSkipped = true;
        // First line is the column header ("time,...") — skip it. CSVLogger
        // writes it before any rows.
        if (line[0] < '0' || line[0] > '9') return;
    }

    // CSVLogger columns:
    //   time,lat,lon,alt,speed,heartrate,cadence,power,calories,np,if,tss,hrzone,pwrzone
    const char* tok[14];
    int nt = 0;
    const char* p = line;
    while (p && nt < 14) {
        tok[nt++] = p;
        const char* c = strchr(p, ',');
        if (!c) break;
        p = c + 1;
    }
    if (nt < 5) return;

    long long e = isoToEpoch(tok[0]);
    float speed = (float)atof(tok[4]);

    if (e > 0) {
        if (!_haveTime) {
            _haveTime = true;
            _firstEpoch = _lastEpoch = _prevEpoch = e;
            _prevSpeed = speed;
        } else {
            if (e > _prevEpoch) {
                // Integrate distance using the previous row's speed over the gap.
                float dt = (float)(e - _prevEpoch);
                _stats.distanceM += (_prevSpeed / 3.6f) * dt;
            }
            _lastEpoch = e;
            _prevEpoch = e;
            _prevSpeed = speed;
        }
    }

    if (speed > _stats.maxSpeedKmh) _stats.maxSpeedKmh = speed;

    if (nt >= 6) { float v = (float)atof(tok[5]); if (v > 0) { _sumHr += v; _hrN++; if (v > _stats.maxHr) _stats.maxHr = v; } }
    if (nt >= 7) { float v = (float)atof(tok[6]); if (v > 0) { _sumCad += v; _cadN++; if (v > _maxCad) _maxCad = v; } }
    if (nt >= 8) { float v = (float)atof(tok[7]); if (v > 0) { _sumPwr += v; _pwrN++; if (v > _stats.maxPower) _stats.maxPower = v; } }
    if (nt >= 9)  _stats.calories = (float)atof(tok[8]);
    if (nt >= 10) _stats.np      = (float)atof(tok[9]);
    if (nt >= 12) _stats.tss     = (float)atof(tok[11]);
}

FitLogParser::FitLogParser() { reset(); }

void FitLogParser::resetLocal() {
    _done     = false;
    _pos      = 0;
    _stats    = RideStats{};
    _fileSize = 0;
}

void FitLogParser::clearDefs() {
    for (int i = 0; i < 16; i++) {
        _defs[i].used       = false;
        _defs[i].globalNum  = 0;
        _defs[i].numFields  = 0;
    }
}

void FitLogParser::reset() {
    resetLocal();
    clearDefs();
    _carryLen = 0;
    if (_file.isOpen()) _file.close();
}

bool FitLogParser::open(IStorage* storage, const char* path) {
    reset();
    if (!storage) return false;
    _file = storage->openFile(path, FILE_READ);
    if (!_file) return false;

    _fileSize = _file.fileSize();
    if (_fileSize < 16) { _file.close(); return false; }

    uint8_t hdr[14];
    if (_file.read(hdr, sizeof(hdr)) != (int)sizeof(hdr)) { _file.close(); return false; }
    if (memcmp(&hdr[8], ".FIT", 4) != 0) { _file.close(); return false; }
    uint8_t headerTotal = hdr[0];
    if (headerTotal > sizeof(hdr)) {
        uint8_t tmp[48];
        int toSkip = headerTotal - sizeof(hdr);
        while (toSkip > 0) {
            int n = toSkip > (int)sizeof(tmp) ? (int)sizeof(tmp) : toSkip;
            if (_file.read(tmp, n) != n) { _file.close(); return false; }
            toSkip -= n;
        }
    }
    _pos = headerTotal;
    clearDefs();
    return _file.isOpen();
}

void FitLogParser::close() {
    if (_file.isOpen()) _file.close();
}

bool FitLogParser::step() {
    if (_done) return false;

    // Top up the persistent carry buffer. Records straddle the chunked file
    // reads all the time (RECORD messages are ~27 B, reads are small), so
    // parsing MUST run over carried bytes -- the previous per-chunk parser
    // dropped any record spanning a chunk boundary and then resumed mid
    // record, desynchronizing the stream (LAP/SESSION values were lost or
    // garbage, so .fit summaries showed wrong/zero numbers).
    bool eof = false;
    if (_carryLen < sizeof(_carry)) {
        const uint16_t want = (uint16_t)(sizeof(_carry) - _carryLen);
        int n = _file.read(_carry + _carryLen, want);
        if (n <= 0) {
            eof = true;
        } else {
            _carryLen += (uint16_t)n;
            if ((uint16_t)n < want) eof = true;  // short read: EOF
        }
    }

    size_t off = 0;
    while (off < _carryLen) {
        const size_t remaining = _carryLen - off;
        const uint8_t recHdr       = _carry[off];
        const bool isDefinition    = (recHdr & 0x40) != 0;
        const bool isTimestamped   = (recHdr & 0x80) != 0; // 1 extra byte (our writer never sets it)
        const uint8_t localType    = recHdr & 0x0F;

        if (isDefinition) {
            // hdr(1) + reserved(1) + arch(1) + global(2) + numFields(1)
            if (remaining < 6) break;
            const uint8_t numFields = _carry[off + 5];
            if (remaining < 6 + (size_t)numFields * 3) break;

            // Architecture: 0 = little-endian (our writer), 1 = big-endian.
            uint16_t globalNum;
            if (_carry[off + 2] == 1)
                globalNum = (uint16_t)(_carry[off + 4] | (_carry[off + 3] << 8));
            else
                globalNum = (uint16_t)(_carry[off + 3] | (_carry[off + 4] << 8));

            LocalDef& ld = _defs[localType];
            ld.used = true;
            ld.globalNum = globalNum;
            // Clamped so the payload-size walk below can never index past
            // fields[32]; anything wider is malformed (or not FIT) means.
            ld.numFields = numFields > 32 ? 32 : numFields;
            for (uint8_t fi = 0; fi < numFields && fi < 32; fi++) {
                const uint8_t* f = &_carry[off + 6 + (size_t)fi * 3];
                ld.fields[fi].num  = f[0];
                ld.fields[fi].size = f[1];
            }
            off += 6 + (size_t)numFields * 3;
        } else {
            const LocalDef& ld = _defs[localType];
            if (!ld.used) {
                // Data record for an undefined local type: its length is
                // unknowable. Our writer always defines before use, so this
                // means desync -- skip one byte to resynchronize.
                off += 1;
                continue;
            }
            uint16_t payloadSize = 0;
            for (uint8_t fi = 0; fi < ld.numFields; fi++)
                payloadSize += ld.fields[fi].size;
            const size_t need = 1 + (size_t)payloadSize + (isTimestamped ? 1 : 0);
            if (remaining < need) break;  // incomplete: keep bytes for next step
            applyDataRecord(recHdr, &_carry[off + 1], payloadSize);
            off += need;
        }
    }

    // Drop the consumed prefix, keeping any partial record for the next call.
    if (off) {
        memmove(_carry, _carry + off, _carryLen - off);
        _carryLen -= (uint16_t)off;
    }

    // Finish once the file is exhausted. Whatever is still held can never be
    // completed -- there is no more input. The usual leftover is the 2-byte
    // trailing FIT CRC, which is not a record at all: the previous test
    // (eof && _carryLen == 0) could never become true for it, so step() kept
    // answering "more to come" forever. On the summary screen that meant
    // _parsing never cleared: the .fit stayed on "Reading..." and its numbers
    // were never shown (CSV/TCX were unaffected -- they are line based).
    //
    // The second condition is the same stall reached with a full buffer: a
    // record larger than _carry makes no progress while there is no room to
    // read more, so stop instead of spinning.
    if (eof || (off == 0 && _carryLen == sizeof(_carry))) {
        _carryLen = 0;
        _done = true;
        _stats.valid = true;
        return false;
    }
    return true;
}

void FitLogParser::applyDataRecord(uint8_t recHdr, const uint8_t* p, size_t /*avail*/) {
    uint8_t localType = recHdr & 0x0F;
    const LocalDef& ld = _defs[localType];
    if (!ld.used) return;
    if (ld.globalNum == 19)      handleLapRecord(ld, p);
    else if (ld.globalNum == 18) handleSessionRecord(ld, p);
}

static uint32_t fitReadU(const uint8_t* p, uint8_t size) {
    uint32_t v = 0;
    for (uint8_t i = 0; i < size && i < 4; i++) v |= (uint32_t)p[i] << (8 * i);
    return v;
}

void FitLogParser::handleLapRecord(const LocalDef& def, const uint8_t* p) {
    _stats.lapCount++;
    uint32_t totalElapsedTime = 0, totalDistance = 0, totalCalories = 0;
    uint8_t avgHr = 0xFF, maxHr = 0xFF, avgCadence = 0xFF;
    uint16_t maxSpeed = 0;

    const uint8_t* base = p;
    for (uint8_t fi = 0; fi < def.numFields; fi++) {
        const FieldMap& fm = def.fields[fi];
        switch (fm.num) {
            case 7:  totalElapsedTime = fitReadU(base, fm.size); break;
            case 9:  totalDistance    = fitReadU(base, fm.size); break;
            case 13: maxSpeed         = (uint16_t)fitReadU(base, fm.size); break;
            case 15: avgHr            = (uint8_t)fitReadU(base, fm.size); break;
            case 16: maxHr            = (uint8_t)fitReadU(base, fm.size); break;
            case 17: avgCadence       = (uint8_t)fitReadU(base, fm.size); break;
            case 32: totalCalories    = fitReadU(base, fm.size); break;
        }
        base += fm.size;
    }

    _stats.distanceM += (float)totalDistance / 100.0f;
    _stats.calories  += (float)totalCalories / 10.0f;
    uint32_t lapSec = totalElapsedTime / 1000;
    if (lapSec > _stats.durationSec) _stats.durationSec = lapSec;

    float spdkmh = (float)maxSpeed / 1000.0f * 3.6f;
    if (spdkmh > _stats.maxSpeedKmh) _stats.maxSpeedKmh = spdkmh;
        if (avgHr != 0xFF && avgHr > (uint8_t)_stats.maxHr) _stats.maxHr = (float)avgHr;
    if (maxHr != 0xFF && maxHr > (uint8_t)_stats.maxHr) _stats.maxHr = (float)maxHr;
    if (avgCadence != 0xFF && avgCadence > (uint8_t)_stats.avgCadence) _stats.avgCadence = (float)avgCadence;
    _stats.valid = true;   // LAP-only files (no SESSION) should still summarize
}

void FitLogParser::handleSessionRecord(const LocalDef& def, const uint8_t* p) {
    uint32_t elapsed = 0, distance = 0;
    const uint8_t* base = p;
    for (uint8_t fi = 0; fi < def.numFields; fi++) {
        const FieldMap& fm = def.fields[fi];
        switch (fm.num) {
            case 7:  elapsed  = fitReadU(base, fm.size); break;
            case 9:  distance = fitReadU(base, fm.size); break;
        }
        base += fm.size;
    }
    _stats.durationSec = elapsed / 1000;
    _stats.distanceM   = (float)distance / 100.0f;
    if (_stats.durationSec > 0)
        _stats.avgSpeedKmh = _stats.distanceM / (float)_stats.durationSec * 3.6f;
    _stats.valid = true;
}

// ---------------------------------------------------------------------------
// TcxLogParser: line-based XML scan for the <Lap> aggregates TCXLogger writes.
// ---------------------------------------------------------------------------
TcxLogParser::TcxLogParser() { reset(); }

void TcxLogParser::reset() {
    _done = false;
    _lineLen = 0;
    _sumHr = 0;
    _sumCadence = 0;
    _hrN = 0;
    _cadN = 0;
    _stats = RideStats{};
    _hrExpect = HrExpect::None;
    if (_file.isOpen()) _file.close();
}

bool TcxLogParser::open(IStorage* storage, const char* path) {
    reset();
    if (!storage) return false;
    _file = storage->openFile(path, FILE_READ);
    return (bool)_file;
}

void TcxLogParser::close() {
    if (_file.isOpen()) _file.close();
    _done = true;
}

bool TcxLogParser::step() {
    char chunk[96];
    int n = _file.read(chunk, sizeof(chunk));
    for (int i = 0; i < n; i++) {
        const char c = chunk[i];
        if (c == '\n') {
            if (_lineLen > 0) { _buf[_lineLen] = 0; processLine(_buf); _lineLen = 0; }
        } else if (c != '\r') {
            if (_lineLen < sizeof(_buf) - 1) _buf[_lineLen++] = c;
        }
    }

    if (n <= 0) {
        if (_lineLen > 0) { _buf[_lineLen] = 0; processLine(_buf); _lineLen = 0; }
        _done = true;
        _stats.avgCadence  = _cadN > 0 ? _sumCadence / (float)_cadN : 0;
        _stats.avgHr       = _hrN  > 0 ? _sumHr      / (float)_hrN  : 0;
        _stats.avgSpeedKmh = _stats.durationSec > 0
                           ? _stats.distanceM / (float)_stats.durationSec * 3.6f : 0;
        _stats.valid = _stats.lapCount > 0;
    }
    return n > 0;
}

float TcxLogParser::extractBetween(const char* line, const char* open, const char* close) {
    const char* start = strstr(line, open);
    if (!start) return -1;
    start += strlen(open);
    const char* end = strstr(start, close);
    if (!end) return -1;
    char buf[32];
    int len = (int)(end - start);
    if (len > 31) len = 31;
    memcpy(buf, start, len);
    buf[len] = 0;
    return (float)atof(buf);
}

void TcxLogParser::processLine(const char* line) {
    float v;

    if (strstr(line, "<Lap ")) {
        _stats.lapCount++;
    }

    if ((v = extractBetween(line, "<TotalTimeSeconds>", "</TotalTimeSeconds>")) >= 0) {
        _stats.durationSec += (uint32_t)v;
    }
    if ((v = extractBetween(line, "<DistanceMeters>", "</DistanceMeters>")) >= 0) {
        _stats.distanceM += v;
    }
    if ((v = extractBetween(line, "<MaximumSpeed>", "</MaximumSpeed>")) >= 0) {
        float kmh = v * 3.6f;  // TCX stores m/s
        if (kmh > _stats.maxSpeedKmh) _stats.maxSpeedKmh = kmh;
    }
    if ((v = extractBetween(line, "<Calories>", "</Calories>")) >= 0) {
        _stats.calories += v;
    }
    if ((v = extractBetween(line, "<Cadence>", "</Cadence>")) >= 0) {
        _sumCadence += v;
        _cadN++;
    }
    if ((v = extractBetween(line, "<AvgNormalizedPower>", "</AvgNormalizedPower>")) >= 0) {
        _stats.np = v;
    }
    if ((v = extractBetween(line, "<Tss>", "</Tss>")) >= 0) {
        _stats.tss = v;
    }

    // HR values are nested: <AverageHeartRateBpm><Value>N</Value></AverageHeartRateBpm>
    // Track which one we're expecting so <Value> updates the right accumulator.
    if (strstr(line, "<AverageHeartRateBpm>")) {
        _hrExpect = HrExpect::Avg;
    } else if (strstr(line, "<MaximumHeartRateBpm>")) {
        _hrExpect = HrExpect::Max;
    } else if ((v = extractBetween(line, "<Value>", "</Value>")) >= 0) {
        if (_hrExpect == HrExpect::Avg) {
            _sumHr += v;
            _hrN++;
        } else if (_hrExpect == HrExpect::Max) {
            if (v > _stats.maxHr) _stats.maxHr = v;
        }
        _hrExpect = HrExpect::None;
    }
}
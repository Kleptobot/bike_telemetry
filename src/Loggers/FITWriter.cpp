#include "FITWriter.hpp"

// CRC-16 table-free bit-by-bit implementation per FIT protocol spec.
// (Garmin's SDK publishes a 16-entry nibble lookup table for speed;
// this bit-by-bit version is simpler to verify correctness first —
// swap to the table version later if CRC becomes a measured bottleneck.)
uint16_t FitWriter::crc16Update(uint16_t crc, uint8_t byte) {
    static const uint16_t table[16] = {
        0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
        0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
    };
    uint16_t tmp = table[crc & 0xF];
    crc = (crc >> 4) & 0x0FFF;
    crc = crc ^ tmp ^ table[byte & 0xF];

    tmp = table[crc & 0xF];
    crc = (crc >> 4) & 0x0FFF;
    crc = crc ^ tmp ^ table[(byte >> 4) & 0xF];

    return crc;
}

void FitWriter::rawWrite(const uint8_t* buf, size_t n) {
    _file.write(buf, n);
    _dataSize += n;
}

bool FitWriter::open(const String& path) {
    // O_RDWR (not just O_WRITE) is required because close() seeks back to
    // patch the header's data_size field after all messages are written.
    _file = _storage->openFile(path.c_str(), O_RDWR | O_CREAT | O_TRUNC);
    if (!_file) return false;

    _dataSize = 0;

    // Write a placeholder 14-byte header; data_size gets patched in close().
    uint8_t header[14] = {0};
    header[0] = 14;          // header_size
    header[1] = 0x20;        // protocol_version (FIT 2.0)
    // profile_version (u16 LE) - update to match the SDK version you target
    uint16_t profileVersion = 2167;
    memcpy(&header[2], &profileVersion, 2);
    // header[4..7] data_size -- patched on close()
    // header[8..11] = ".FIT"
    header[8]  = '.';
    header[9]  = 'F';
    header[10] = 'I';
    header[11] = 'T';
    // header[12..13] header_crc left as 0x0000 (optional, valid)

    // Header bytes are deliberately NOT fed into a CRC here. At this point
    // data_size (bytes 4-7) is a placeholder (0) -- the real value is only
    // known once all messages are written. close() recomputes the CRC in
    // one pass over the FINAL on-disk bytes (real header + all data), which
    // is the only way to guarantee the stored CRC matches what's on disk.
    _file.write(header, 14);

    return true;
}

void FitWriter::writeDefinition(uint8_t localType, uint16_t globalMesgNum,
                                 const FitFieldDef* fields, uint8_t numFields) {
    uint8_t recHeader = 0x40 | (localType & 0x0F); // definition message
    rawWrite(&recHeader, 1);

    uint8_t fixed[5];
    fixed[0] = 0x00; // reserved
    fixed[1] = 0x00; // architecture: little-endian
    memcpy(&fixed[2], &globalMesgNum, 2);
    fixed[4] = numFields;
    rawWrite(fixed, 5);

    for (uint8_t i = 0; i < numFields; ++i) {
        uint8_t f[3] = { fields[i].num, fields[i].size, fields[i].baseType };
        rawWrite(f, 3);
    }
}

void FitWriter::beginDataMessage(uint8_t localType) {
    uint8_t recHeader = 0x00 | (localType & 0x0F); // data message, normal header
    rawWrite(&recHeader, 1);
}

void FitWriter::writeU8(uint8_t v)   { rawWrite(&v, 1); }
void FitWriter::writeU16(uint16_t v) { rawWrite(reinterpret_cast<uint8_t*>(&v), 2); }
void FitWriter::writeU32(uint32_t v) { rawWrite(reinterpret_cast<uint8_t*>(&v), 4); }
void FitWriter::writeS16(int16_t v)  { rawWrite(reinterpret_cast<uint8_t*>(&v), 2); }
void FitWriter::writeS32(int32_t v)  { rawWrite(reinterpret_cast<uint8_t*>(&v), 4); }

void FitWriter::writeString(const char* s, uint8_t fieldSize) {
    uint8_t buf[64] = {0}; // FIT strings are short; 64 is generous headroom
    uint8_t len = fieldSize > 64 ? 64 : fieldSize;
    strncpy(reinterpret_cast<char*>(buf), s, len - 1);
    rawWrite(buf, len);
}

void FitWriter::close() {
    // _dataSize is exactly "bytes written after the 14-byte header", so the
    // end-of-data position is always 14 + _dataSize.
    const uint32_t endOfDataPos = 14 + _dataSize;

    // Patch data_size (bytes 4-7 of header) now that we know the total.
    _file.seekSet(4);
    uint8_t sizeBytes[4];
    memcpy(sizeBytes, &_dataSize, 4);
    _file.write(sizeBytes, 4);

    // Compute the CRC in a single pass over the FINAL on-disk bytes (header
    // with the real data_size already patched in, plus all data messages).
    // Doing this here -- rather than incrementally during each write --
    // guarantees the CRC matches exactly what ends up on disk, regardless
    // of when data_size becomes known. Read back in fixed-size chunks
    // rather than the whole file at once to keep RAM usage bounded.
    _file.seekSet(0);
    uint16_t crc = 0;
    uint8_t buf[64];
    uint32_t remaining = endOfDataPos;
    while (remaining > 0) {
        size_t chunkLen = remaining < sizeof(buf) ? remaining : sizeof(buf);
        int n = _file.read(buf, chunkLen);
        if (n <= 0) break; // read error -- shouldn't happen, but don't spin
        for (int i = 0; i < n; ++i) {
            crc = crc16Update(crc, buf[i]);
        }
        remaining -= n;
    }
    _crc = crc;

    // Return to end-of-data before appending the trailing CRC.
    _file.seekSet(endOfDataPos);

    uint8_t crcBytes[2];
    memcpy(crcBytes, &_crc, 2);
    _file.write(crcBytes, 2);

    _file.close();
}
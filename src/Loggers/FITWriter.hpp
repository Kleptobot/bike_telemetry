#ifndef FITWRITER_H
#define FITWRITER_H

#include <Arduino.h>
#include <stdint.h>
#include "HAL/StorageInterface.hpp"

// ---------------------------------------------------------------------------
// Minimal FIT binary encoder.
// Only the message types/fields needed for a basic ride activity file:
// FILE_ID, DEVICE_INFO, RECORD, LAP, SESSION, ACTIVITY.
//
// Field numbers / base types / scale factors below are taken from Garmin's
// FIT SDK Profile.xlsx (global profile), NOT invented. If you add fields,
// look up the real field number + base type + scale/offset there, or
// platforms will silently misinterpret your data.
// ---------------------------------------------------------------------------

// ---- FIT base types (subset) ----
namespace FitBaseType {
    constexpr uint8_t ENUM    = 0x00;
    constexpr uint8_t UINT8   = 0x02;
    constexpr uint8_t SINT16  = 0x83;
    constexpr uint8_t UINT16  = 0x84;
    constexpr uint8_t SINT32  = 0x85;
    constexpr uint8_t UINT32  = 0x86;
    constexpr uint8_t STRING  = 0x07;
    constexpr uint8_t UINT8Z  = 0x0A;
    constexpr uint8_t UINT16Z = 0x8B;
    constexpr uint8_t UINT32Z = 0x8C;
}

// ---- FIT global message numbers (subset) ----
namespace FitMesg {
    constexpr uint16_t FILE_ID      = 0;
    constexpr uint16_t DEVICE_INFO  = 23;
    constexpr uint16_t LAP          = 19;
    constexpr uint16_t RECORD       = 20;
    constexpr uint16_t SESSION      = 18;
    constexpr uint16_t ACTIVITY     = 34;
}

// One field within a definition message: {field_num, size_bytes, base_type}
struct FitFieldDef {
    uint8_t num;
    uint8_t size;
    uint8_t baseType;
};

// ---------------------------------------------------------------------------
// FitWriter: stateful binary writer. Buffers nothing in RAM beyond the
// current message — writes straight to the open SD file, patches the
// header's data_size and appends CRC on close().
// ---------------------------------------------------------------------------
class FitWriter {
public:
    // storage must outlive the FitWriter (typically owned by the firmware's
    // top-level app/context and injected at construction).
    explicit FitWriter(IStorage* storage) : _storage(storage) {}

    bool open(const String& path);
    void close(); // patches header size, writes trailing CRC, closes file

    // Writes a definition message (call once per local type before first use,
    // or again if the field layout changes).
    void writeDefinition(uint8_t localType, uint16_t globalMesgNum,
                          const FitFieldDef* fields, uint8_t numFields);

    // Begins a data message for a given local type (caller then calls
    // writeU8/writeU16/... in the SAME ORDER as the matching definition).
    void beginDataMessage(uint8_t localType);

    void writeU8(uint8_t v);
    void writeU16(uint16_t v);
    void writeU32(uint32_t v);
    void writeS16(int16_t v);
    void writeS32(int32_t v);
    void writeString(const char* s, uint8_t fieldSize); // pads/truncates to fieldSize

private:
    IStorage* _storage;
    File32 _file;
    uint32_t _dataSize = 0; // bytes written after header, before CRC
    uint16_t _crc = 0;

    void rawWrite(const uint8_t* buf, size_t n);
    static uint16_t crc16Update(uint16_t crc, uint8_t byte);
};

// FIT epoch is 1989-12-31 00:00:00 UTC, NOT unix epoch.
// Subtract this many seconds from a unix timestamp before writing.
constexpr long long FIT_EPOCH_OFFSET = 631065600LL;

// Takes a long long since timeData::unixtime() returns long long; FIT
// timestamps themselves are always uint32_t on the wire.
inline uint32_t toFitTimestamp(long long unixSeconds) {
    return static_cast<uint32_t>(unixSeconds - FIT_EPOCH_OFFSET);
}

// Lat/lon degrees -> FIT semicircles (sint32)
inline int32_t toSemicircles(double degrees) {
    return static_cast<int32_t>(degrees * (2147483648.0 / 180.0));
}

#endif /* FITWRITER_H */
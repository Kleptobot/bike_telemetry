// Host-side SdFat stub backed by real files under ./sdcard/.
//
// This is the highest-value stub in the set. Because File32 writes to an
// actual filesystem, a simulated ride produces a genuine .fit on disk that can
// be opened in Garmin Connect, Strava or the FIT SDK -- which is a far
// stronger check on the logger than any assertion, and is exactly how the
// "rides logged 3.6x too fast" defect would have been caught.
#ifndef SDFAT_H_STUB
#define SDFAT_H_STUB

#include "Arduino.h"
#include <cstdio>
#include <string>

typedef uint8_t oflag_t;

#define O_RDONLY  0x01
#define O_WRITE   0x02
#define O_RDWR    0x04
#define O_CREAT   0x08
#define O_TRUNC   0x10
#define O_APPEND  0x20
#define O_READ    O_RDONLY
#define FILE_READ  O_RDONLY
#define FILE_WRITE (O_RDWR | O_CREAT | O_APPEND)

// FAT date/time packing, same layout the real library uses.
#define FS_DATE(y, m, d)  ((uint16_t)((((y) - 1980) << 9) | ((m) << 5) | (d)))
#define FS_TIME(h, m, s)  ((uint16_t)(((h) << 11) | ((m) << 5) | ((s) >> 1)))

/** Root of the simulated card on the host filesystem. */
const char* simSdRoot();
void        simSetSdRoot(const char* path);

/** Maps a device path ("/activity_1.fit") to a host path. */
std::string simSdHostPath(const char* devicePath);

class File32 : public Print {
public:
    File32() {}
    ~File32() { close(); }

    // Non-copyable, movable -- the firmware returns File32 by value from
    // IStorage::openFile, so move semantics have to work.
    File32(const File32&) = delete;
    File32& operator=(const File32&) = delete;
    File32(File32&& o) noexcept : _fp(o._fp) { o._fp = nullptr; }
    File32& operator=(File32&& o) noexcept {
        if (this != &o) { close(); _fp = o._fp; o._fp = nullptr; }
        return *this;
    }

    bool open(const char* path, oflag_t mode = O_RDONLY);
    void close();
    bool isOpen() const { return _fp != nullptr; }
    explicit operator bool() const { return isOpen(); }

    size_t write(uint8_t c) override { return _fp ? fwrite(&c, 1, 1, _fp) : 0; }
    size_t write(const uint8_t* buf, size_t n) override { return _fp ? fwrite(buf, 1, n, _fp) : 0; }
    size_t write(const void* buf, size_t n) { return write((const uint8_t*)buf, n); }

    int  read(void* buf, size_t n) { return _fp ? (int)fread(buf, 1, n, _fp) : -1; }
    int  read();
    bool seekSet(uint32_t pos) { return _fp && fseek(_fp, (long)pos, SEEK_SET) == 0; }
    uint32_t curPosition() const { return _fp ? (uint32_t)ftell(_fp) : 0; }
    uint32_t fileSize();
    int  available();
    int  peek();
    void flush() { if (_fp) fflush(_fp); }

private:
    FILE* _fp = nullptr;
};

class SdFat32 {
public:
    bool begin(uint8_t csPin = 0);
    void end() { _mounted = false; }
    bool exists(const char* path);
    bool remove(const char* path);
    bool mkdir(const char* path, bool pFlag = true);

private:
    bool _mounted = false;
};

/** Matches the real library's timestamp callback registration. */
class FsDateTime {
public:
    typedef void (*callback_t)(uint16_t* date, uint16_t* time, uint8_t* ms10);
    static void setCallback(callback_t cb) { _cb = cb; }
    static callback_t callback() { return _cb; }
private:
    static callback_t _cb;
};

#endif /* SDFAT_H_STUB */

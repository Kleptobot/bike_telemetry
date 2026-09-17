// Host-side TinyUSB MSC stub.
//
// The firmware's USBMSC wrapper must compile on the host so the whole App
// still builds during development, but USB enumeration/transfers cannot run
// here. Keep the same method surface the wrapper uses so the wrapper itself
// is the code under test; all operations are no-ops.
#ifndef ADAFRUIT_TINYUSB_STUB_H
#define ADAFRUIT_TINYUSB_STUB_H

#include <cstdint>

class Adafruit_USBD_MSC {
public:
    typedef int32_t (*read_callback_t)(uint32_t lba, void* buffer,
                                       uint32_t bufsize);
    typedef int32_t (*write_callback_t)(uint32_t lba, uint8_t* buffer,
                                        uint32_t bufsize);
    typedef void (*flush_callback_t)(void);
    typedef bool (*ready_callback_t)(void);

    Adafruit_USBD_MSC() {}

    // Interface is "registered" exactly once by convention (see USBMSC.cpp).
    bool begin() { _begun = true; ++_beginCount; return true; }
    bool begun() const { return _begun; }
    int  beginCount() const { return _beginCount; }

    void setID(const char*, const char*, const char*) {}
    void setCapacity(uint32_t, uint16_t) {}
    void setUnitReady(bool) {}
    void setReadWriteCallback(read_callback_t, write_callback_t,
                              flush_callback_t) {}
    void setReadyCallback(ready_callback_t) {}

private:
    bool _begun = false;
    int  _beginCount = 0;
};

#endif /* ADAFRUIT_TINYUSB_STUB_H */
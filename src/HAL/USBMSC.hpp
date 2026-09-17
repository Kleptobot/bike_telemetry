#ifndef USBMSC_H
#define USBMSC_H

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

#include "SDCard.hpp"

/**
 * @class USBMSC
 * @brief Exposes the SD card as a USB Mass Storage device.
 *
 * Wraps Adafruit_USBD_MSC and delegates block-level reads/writes to the
 * underlying SdCard. The interface is only meaningful while the SD card is
 * mounted and the App is in the IDLE state -- call begin()/end() to toggle
 * unit-ready (which the host sees as mount/unmount).
 *
 * Lifecycle:
 *   - Construct with a pointer to the SDCardSystem.
 *   - begin() registers the interface with TinyUSB exactly once (across the
 *     whole run) and sets unit-ready. Repeated calls only re-assert ready.
 *   - end() clears unit-ready. The interface is never re-registered, because
 *     Adafruit_USBD_MSC::begin()->TinyUSBDevice.addInterface() appends to a
 *     fixed-size descriptor buffer; calling it more than once corrupts the
 *     configuration descriptor and crashes the device on the next host
 *     enumeration.
 *
 * Threading: the TinyUSB callbacks run on the USB RTOS task and perform the
 * SdFat/SPI I/O synchronously, guarded by SpiBusLock (see SpiBusLock.hpp) --
 * the same lock the display flush holds -- because the SD card and the
 * ST7789 share one SPI bus. Returning 0 ("not ready") from the callbacks is
 * NOT an option: msc_device.c then queues a fake transfer-complete event,
 * spinning the high-priority USB task and starving the main loop.
 * The USB task stack was raised (tools/patch_tinyusb_stack.py) so the SdFat
 * call chain cannot overflow.
 */
class USBMSC {
public:
    USBMSC()
        : _sdSystem(nullptr), _enabled(false), _registered(false),
          _inFlight(0) {}

    /** Provide the SD card system. Must be called before begin(). */
    void setSDCardSystem(SDCardSystem* sdSystem) { _sdSystem = sdSystem; }

    /** Register the MSC interface (once) and assert unit-ready. */
    void begin();

    /** Clear unit-ready so the host drops the volume. */
    void end();

    /**
     * Suspend MSC so the main thread can safely use SdFat (e.g. the Logs
     * screens): clears unit-ready, then waits (bounded) until any in-flight
     * block transfer has finished. After pause() returns, the USB task no
     * longer touches the card.
     */
    void pause();

    /** Re-assert unit-ready after pause() (no-op if already running). */
    void resume();

    /** True while unit-ready is asserted. */
    bool isEnabled() const { return _enabled; }

private:
    static constexpr uint16_t SECTOR_SIZE = 512;

    SDCardSystem* _sdSystem;
    Adafruit_USBD_MSC _msc;
    bool _enabled;
    bool _registered;  // true once the interface has been added to TinyUSB

    // Number of callbacks currently inside card I/O (USB task side). The
    // main thread waits for this to reach 0 in pause() before touching SdFat.
    volatile uint8_t _inFlight;

    // TinyUSB callbacks (static, run on the USB task).
    static int32_t _read_cb(uint32_t lba, void* buffer, uint32_t bufsize);
    static int32_t _write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
    static void _flush_cb();
    static bool _ready_cb();

    static USBMSC* _instance;
};

#endif /* USBMSC_H */

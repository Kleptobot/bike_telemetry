#include "USBMSC.hpp"
#include "SpiBusLock.hpp"

#if defined(ARDUINO_ARCH_NRF52)
SemaphoreHandle_t SpiBusLock::_mutex = nullptr;
#endif

USBMSC* USBMSC::_instance = nullptr;

void USBMSC::begin() {
    if (_enabled) return;
    if (!_sdSystem || !_sdSystem->isMounted()) return;

    SdCard* card = _sdSystem->card();
    if (!card) return;

    // The MSC callbacks run on the USB task and touch the SPI bus the display
    // also uses; make sure the shared-bus mutex exists before any transfer.
    SpiBusLock::init();

    // Register this instance as the callback target. Only one USBMSC
    // exists (owned by the singleton App), so this is safe.
    _instance = this;

    // Register the interface with TinyUSB exactly once. addInterface()
    // appends the MSC interface to a fixed 256-byte descriptor buffer, so a
    // second registration (e.g. IDLE -> CONFIG -> IDLE) would overflow /
    // corrupt the configuration descriptor and crash the device when the
    // host next enumerates it. Toggling unit-ready is enough to mount/unmount.
    if (!_registered) {
        _msc.setID("OBike", "SD Card", "1.0");
        _msc.setCapacity(card->sectorCount(), 512);
        _msc.setReadWriteCallback(_read_cb, _write_cb, _flush_cb);
        _msc.setReadyCallback(_ready_cb);
        _msc.begin();
        _registered = true;
    }

    _msc.setUnitReady(true);
    _enabled = true;
}

void USBMSC::end() {
    if (!_enabled) return;
    // There is no Adafruit_USBD_MSC::end(); the interface stays registered
    // but we signal "not ready" so the host drops the volume.
    _msc.setUnitReady(false);
    _enabled = false;
    // _instance is intentionally NOT cleared here: the USB task may still be
    // mid-transfer, and invalidating the dispatch target would crash it.
}

void USBMSC::pause() {
    end();
    // Drain: wait until any in-flight block transfer has left the callbacks.
    // end() makes further callbacks return -1 without touching the card, so
    // at most the one transfer that was already inside can be outstanding.
    // delay(1) yields so the USB task (higher priority) can finish.
    for (int i = 0; _inFlight && i < 250; ++i) delay(1);
    if (_inFlight)
        Serial.println("[USBMSC] WARNING: in-flight transfer did not drain");
}

void USBMSC::resume() {
    // The host may have rewritten the card while the volume was exposed.
    // SdFat's caches are stale after that, so force a clean remount before
    // the firmware touches the filesystem again. Safe here: MSC is paused,
    // no transfer is in flight, and the Logs screens close their files on
    // exit (LogSummaryScreen::onExit).
    if (_sdSystem) _sdSystem->remount();
    begin();
}

// ---------------------------------------------------------------------------
// TinyUSB MSC callbacks (run on the USB RTOS task).
//
// The SD card and the ST7789 display share one hardware SPI bus, so every
// card access here is guarded by SpiBusLock, which the display's flush also
// holds (see Display.cpp). Blocking on the mutex yields the CPU, unlike
// returning 0 from the callback -- msc_device.c answers that by immediately
// queueing a fake transfer-complete event, which spins the high-priority USB
// task and starves the main loop (frozen screen, mount never completes).
//
// With CFG_TUD_MSC_EP_BUFSIZE = 4096 (tools/patch_tinyusb_stack.py) each
// callback moves up to 8 sectors in one SPI burst. The USB task stack was
// raised to 1536 words in the same patch so the SdFat call chain fits.
// ---------------------------------------------------------------------------

int32_t USBMSC::_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
    if (!_instance || !_instance->_enabled) return -1;
    SdCard* card = _instance->_sdSystem ? _instance->_sdSystem->card() : nullptr;
    if (!card || bufsize < SECTOR_SIZE) return -1;

    SpiBusGuard guard;  // serialize with the display flush on the shared bus
    _instance->_inFlight++;  // pause() waits for this to drain
    size_t ns = bufsize / SECTOR_SIZE;
    int32_t rc = card->readSectors(lba, static_cast<uint8_t*>(buffer), ns)
                     ? static_cast<int32_t>(bufsize)
                     : -1;
    _instance->_inFlight--;
    return rc;
}

int32_t USBMSC::_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
    if (!_instance || !_instance->_enabled) return -1;
    SdCard* card = _instance->_sdSystem ? _instance->_sdSystem->card() : nullptr;
    if (!card || bufsize < SECTOR_SIZE) return -1;

    SpiBusGuard guard;
    _instance->_inFlight++;
    size_t ns = bufsize / SECTOR_SIZE;
    int32_t rc = card->writeSectors(lba, buffer, ns)
                     ? static_cast<int32_t>(bufsize)
                     : -1;
    _instance->_inFlight--;
    return rc;
}

void USBMSC::_flush_cb() {
    // SdFat writes are synchronous; nothing to flush.
}

bool USBMSC::_ready_cb() {
    if (!_instance) return false;
    // Only report ready when we are explicitly enabled (i.e. App is IDLE)
    // and the card is still mounted.
    return _instance->_enabled &&
           _instance->_sdSystem &&
           _instance->_sdSystem->isMounted();
}

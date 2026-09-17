#ifndef SPIBUSLOCK_H
#define SPIBUSLOCK_H

// Serialize access to the single hardware SPI bus shared by the ST7789
// display (main loop) and the SD card's block interface (USB MSC task).
//
// Without this, the USB task's SdFat transactions interleave with the
// display's Adafruit_SPITFT transactions mid-frame and garble the screen.
//
// A FreeRTOS mutex (not a spinlock) is essential here: the USB task runs at
// TASK_PRIO_HIGH, so a busy-wait on a flag held by the lower-priority main
// loop would starve it. Blocking on a mutex yields, and FreeRTOS priority
// inheritance boosts the holder, so neither side can starve the other. The
// display hold time is bounded (one dirty-rect flush), so worst case is a
// short, clean wait.
//
// On the host simulator there is one thread, so the lock compiles to no-ops.

#if defined(ARDUINO_ARCH_NRF52)

#include <FreeRTOS.h>
#include <semphr.h>

class SpiBusLock {
public:
    static void init() {
        if (!_mutex) _mutex = xSemaphoreCreateMutex();
    }
    static void take() {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
    }
    static void give() {
        if (_mutex) xSemaphoreGive(_mutex);
    }

private:
    static SemaphoreHandle_t _mutex;
};

#else  // host simulator: single-threaded no-ops

class SpiBusLock {
public:
    static void init() {}
    static void take() {}
    static void give() {}
};

#endif

// Scope guard so callback/flush paths cannot leak the lock on early return.
class SpiBusGuard {
public:
    SpiBusGuard() { SpiBusLock::take(); }
    ~SpiBusGuard() { SpiBusLock::give(); }
    SpiBusGuard(const SpiBusGuard&) = delete;
    SpiBusGuard& operator=(const SpiBusGuard&) = delete;
};

#endif /* SPIBUSLOCK_H */

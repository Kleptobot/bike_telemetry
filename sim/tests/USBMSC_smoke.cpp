// Host smoke test for the USBMSC synchronous block-transfer callbacks.
//
// The real device can't be driven from here (no USB enumeration), so this
// exercises the things that *can* be validated on the host and that are the
// substance of the robustness fix:
//   1. The MSC interface is registered with TinyUSB exactly once, even across
//      begin()/end()/begin() toggles (the original code re-added the interface
//      every IDLE entry, corrupting the descriptor and crashing on plug-in).
//   2. The read/write callbacks perform the card I/O synchronously and return
//      the byte count directly (no fake-event spin, no main-loop pump).
//
// Build (from sim/):
//   g++ -std=gnu++17 -DARDUINO=10800  -Istubs -I../src
//       tests/USBMSC_smoke.cpp ../src/HAL/USBMSC.cpp ../src/HAL/SDCard.cpp
//       stubs/Arduino.cpp stubs/SdFat.cpp -o build/obike-msc-smoke
//
// (SDCard.cpp is required: resume() calls SDCardSystem::remount() to drop the
// filesystem's cached FAT state after the host has written the volume.)
//
// #define private public is a test-only hack to reach the private static
// callbacks and the private _msc member; it does not affect the firmware.

#define private public
#include "HAL/USBMSC.hpp"
#undef private

#include <cstring>
#include <cstdio>

static int failures = 0;
static void check(bool cond, const char* what) {
    if (!cond) { ++failures; printf("[USBMSC smoke] FAIL: %s\n", what); }
}

int main(int argc, char** argv) {
    // A real file as the "card image"; the SdCard stub reads/writes it directly.
    const char* card = (argc > 1) ? argv[1] : "msc-smoke.bin";
    FILE* f = fopen(card, "w+b");
    if (!f) { printf("[USBMSC smoke] cannot create %s\n", card); return 2; }
    fclose(f);
    simSetSdRoot(card);

    SDCardSystem sdSys;
    if (!sdSys.init(nullptr)) { printf("[USBMSC smoke] sd init failed\n"); return 2; }

    USBMSC u;
    u.setSDCardSystem(&sdSys);

    // Register once --- even across end()/begin() toggles.
    u.begin();
    check(u._msc.beginCount() == 1, "interface registered on first begin");
    u.end();
    u.begin();
    check(u._msc.beginCount() == 1, "interface NOT re-registered after begin/end/begin");

    // Arrange known data at sector 1 directly on the card.
    uint8_t seed[512];
    for (int i = 0; i < 512; ++i) seed[i] = (uint8_t)i;
    check(sdSys.card()->writeSectors(1, seed, 1), "seed sector image");

    // ---- READ (synchronous, SPI-bus-locked) ----
    check(u.isEnabled(), "unit ready after begin");
    uint8_t out[512];
    int32_t r = USBMSC::_read_cb(1, out, 512);
    check(r == 512, "read_cb returns full size synchronously");
    check(memcmp(out, seed, 512) == 0, "read data matches SD image");

    // ---- WRITE (synchronous) ----
    uint8_t wdata[512];
    for (int i = 0; i < 512; ++i) wdata[i] = (uint8_t)(511 - i);
    int32_t w = USBMSC::_write_cb(2, wdata, 512);
    check(w == 512, "write_cb returns full size synchronously");

    // An 8-sector request (CFG_TUD_MSC_EP_BUFSIZE = 4096) succeeds too. Seed
    // 8 sectors at LBA 16 first so the burst lands inside the card image.
    uint8_t big[4096];
    for (int i = 0; i < 4096; ++i) big[i] = (uint8_t)(i * 7);
    check(sdSys.card()->writeSectors(16, big, 8), "seed 8-sector burst");
    uint8_t bigout[4096];
    int32_t r8 = USBMSC::_read_cb(16, bigout, 4096);
    check(r8 == 4096, "read_cb handles an 8-sector burst");
    check(memcmp(bigout, big, 4096) == 0, "8-sector burst data matches");

    // Repeated calls must stay correct (the driver may issue many in a row).
    int32_t r2 = USBMSC::_read_cb(1, out, 512);
    check(r2 == 512 && memcmp(out, seed, 512) == 0,
          "repeated reads are stable");

    uint8_t verify[512];
    check(sdSys.card()->readSectors(2, verify, 1), "read back written sector");
    check(memcmp(verify, wdata, 512) == 0, "written data persisted to SD image");

    // ---- pause() drains and blocks, resume() re-enables ----
    // (pause() is what the App calls before main-thread SdFat access on the
    // Logs screens; it must guarantee the USB task is out of the card.)
    u.resume();
    check(u.isEnabled(), "resume() re-enables after end");
    r = USBMSC::_read_cb(1, out, 512);
    check(r == 512, "read works after resume");
    u.pause();
    check(!u.isEnabled(), "pause() clears unit-ready");
    check(u._inFlight == 0, "no transfer left in flight after pause");
    check(USBMSC::_read_cb(1, out, 512) == -1, "callbacks refuse while paused");
    u.resume();
    check(u.isEnabled(), "resume() works after pause");

    // ---- unit-ready toggle does not disturb registration ----
    u.end();
    check(!u.isEnabled(), "not ready after end");
    check(u._msc.beginCount() == 1, "still exactly one interface registration");

    if (failures == 0) printf("[USBMSC smoke] OK\n");
    remove(card);
    return failures ? 1 : 0;
}
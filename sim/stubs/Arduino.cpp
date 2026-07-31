#include "Arduino.h"
#include <cstdarg>

// ---------------------------------------------------------------------------
// Simulated clock.
//
// Deliberately not the wall clock. The firmware's main loop is full of
// millis() deltas -- render throttling, I2C state machine dwell times, button
// hold detection -- and driving them from an explicit counter means a scenario
// can be replayed deterministically, stepped one tick at a time in a debugger,
// or run far faster than real time. The SDL frontend advances it from the
// frame timer; the unit tests advance it by hand.
// ---------------------------------------------------------------------------
static uint32_t g_millis = 0;

uint32_t millis() { return g_millis; }
uint32_t micros() { return g_millis * 1000u; }

void simSetMillis(uint32_t ms) { g_millis = ms; }
void simAdvanceMillis(uint32_t deltaMs) { g_millis += deltaMs; }

// delay() advances simulated time rather than sleeping. Firmware startup calls
// delay(500) and delay(200) in a few places; sleeping for real would just make
// the simulator boot slowly for no benefit.
void delay(uint32_t ms) { g_millis += ms; }
void delayMicroseconds(uint32_t us) { g_millis += us / 1000u; }

// ---------------------------------------------------------------------------
// GPIO / ADC. The real pin state that matters (buttons, SD detect) is supplied
// by the simulated InputSystem, not through these.
// ---------------------------------------------------------------------------
void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t, uint8_t) {}
int  digitalRead(uint8_t) { return 0; }
void analogReadResolution(uint8_t) {}

// Battery ADC. Returns a count corresponding to a nominal 3.9 V cell so the
// battery widget shows something plausible and moves when a scenario changes
// it. Matches the divider maths in SensorSystem::update:
//   vBat = count * VBAT_MV_PER_LBS * (1510/510)
static int g_batteryAdc = 388;   // ~3.9 V

void simSetBatteryAdc(int count) { g_batteryAdc = count; }
int  analogRead(uint8_t) { return g_batteryAdc; }

// ---------------------------------------------------------------------------
// Print
// ---------------------------------------------------------------------------
size_t Print::print(int v, int base)           { return print((long)v, base); }
size_t Print::print(unsigned int v, int base)  { return print((unsigned long)v, base); }

size_t Print::print(long v, int base) {
    char b[64];
    if (base == 16)      snprintf(b, sizeof(b), "%lX", (unsigned long)v);
    else if (base == 2)  { String s(v, 2); return write(s.c_str()); }
    else                 snprintf(b, sizeof(b), "%ld", v);
    return write(b);
}

size_t Print::print(unsigned long v, int base) {
    char b[64];
    if (base == 16) snprintf(b, sizeof(b), "%lX", v);
    else            snprintf(b, sizeof(b), "%lu", v);
    return write(b);
}

size_t Print::print(double v, int decimals) {
    char b[64];
    snprintf(b, sizeof(b), "%.*f", decimals, v);
    return write(b);
}

size_t Print::print(const String& s)   { return write(s.c_str()); }
size_t Print::println()                { return write("\r\n"); }
size_t Print::println(const String& s) { size_t n = print(s); return n + println(); }

// ---------------------------------------------------------------------------
// Serial
// ---------------------------------------------------------------------------
SerialStub Serial;

static NRF_POWER_Type g_power = {0};
NRF_POWER_Type* NRF_POWER = &g_power;

// Nominal figures so the boot banner prints something sensible.
uint32_t dbgHeapTotal() { return 237568; }
uint32_t dbgHeapUsed()  { return 18492; }

size_t Print::print(const Printable& p) { return p.printTo(*this); }

size_t Print::print(unsigned long long v, int base) {
    char b[64];
    if (base == 16) snprintf(b, sizeof(b), "%llX", v);
    else            snprintf(b, sizeof(b), "%llu", v);
    return write(b);
}

int SerialStub::printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return n;
}

void SerialStub::printBufferReverse(const uint8_t* buf, int len, char sep) {
    for (int i = len - 1; i >= 0; i--) {
        printf("%02X", buf[i]);
        if (i) fputc(sep, stdout);
    }
}

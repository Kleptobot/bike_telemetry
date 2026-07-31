// Host-side Arduino compatibility layer for the OBike simulator and unit tests.
//
// This is deliberately NOT a general-purpose Arduino emulation. It implements
// exactly the surface the firmware and its vendored libraries (Adafruit_GFX,
// ArduinoJson, TinyGPSPlus) actually touch. If something is missing the build
// breaks loudly, which is the behaviour we want -- a silently wrong stub is
// worse than no stub, because the whole point of the simulator is that host
// behaviour matches device behaviour.
#ifndef ARDUINO_H_STUB
#define ARDUINO_H_STUB

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <algorithm>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// Flash-residency macros. On ARM these place data in .rodata; on the host they
// are no-ops and the pgm_read_* accessors are plain dereferences.
// ---------------------------------------------------------------------------
#define PROGMEM
#define PGM_P const char*
#define pgm_read_byte(addr)     (*(const uint8_t*)(addr))
#define pgm_read_word(addr)     (*(const uint16_t*)(addr))
#define pgm_read_dword(addr)    (*(const uint32_t*)(addr))
#define pgm_read_byte_near(addr) pgm_read_byte(addr)
#define memcpy_P                memcpy
#define strcpy_P                strcpy
#define strlen_P                strlen
// Opaque type used for the F("...") flash-string overloads. A forward
// declaration is enough: every use is behind a pointer, and on the host the
// F() macro yields a plain const char* which binds to the other overload.
class __FlashStringHelper;
#define F(s)                    (s)
#define ARDUINO 10800

// min/max/constrain as function templates, NOT macros.
//
// Arduino defines these as macros, which is fine on a bare AVR/ARM core but
// breaks libstdc++ on the host -- std::vector's internals call max(), and a
// macro mangles them into a syntax error. Templates with common_type still
// accept the mixed-type calls the firmware makes (min(uint16_t, int) and
// similar) while leaving the standard library alone.
//
// abs is deliberately not redefined: <cmath>/<cstdlib> already provide
// correctly-typed overloads, and shadowing them with a macro breaks the
// float versions.
template <typename T, typename U>
constexpr typename std::common_type<T, U>::type min(T a, U b) {
    using R = typename std::common_type<T, U>::type;
    return (R)a < (R)b ? (R)a : (R)b;
}
template <typename T, typename U>
constexpr typename std::common_type<T, U>::type max(T a, U b) {
    using R = typename std::common_type<T, U>::type;
    return (R)a > (R)b ? (R)a : (R)b;
}
template <typename T, typename L, typename H>
constexpr typename std::common_type<T, L, H>::type constrain(T x, L lo, H hi) {
    using R = typename std::common_type<T, L, H>::type;
    return (R)x < (R)lo ? (R)lo : ((R)x > (R)hi ? (R)hi : (R)x);
}
using std::abs;

#define HIGH 1
#define LOW  0
#define INPUT           0x0
#define OUTPUT          0x1
#define INPUT_PULLUP    0x2
#define INPUT_PULLDOWN  0x3
#define INPUT_PULLDOWN_SENSE 0x4
#define DEC 10
#define HEX 16
#define BIN 2

// --- XIAO nRF52840 variant pin names -------------------------------------
// Only the ones the firmware references.
enum : uint8_t {
    D0 = 0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10,
    PIN_VBAT = 32, VBAT_ENABLE = 33
};

// SYSTEMOFF is written directly by main.cpp's sleep path.
struct NRF_POWER_Type { uint32_t SYSTEMOFF; };
extern NRF_POWER_Type* NRF_POWER;

// Heap introspection the firmware prints at boot.
uint32_t dbgHeapTotal();
uint32_t dbgHeapUsed();

#ifndef TWO_PI
#define TWO_PI (2.0 * PI)
#endif
#ifndef HALF_PI
#define HALF_PI (PI / 2.0)
#endif
template <typename T> constexpr T sq(T x) { return x * x; }

typedef uint8_t byte;
typedef unsigned int uint;
inline uint32_t bit(uint8_t b) { return 1UL << b; }

// Angle helpers Adafruit_GFX expects from the Arduino core.
#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
inline float radians(float deg) { return deg * (float)(PI / 180.0); }
inline float degrees(float rad) { return rad * (float)(180.0 / PI); }
inline long  map(long x, long inMin, long inMax, long outMin, long outMax) {
    return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

// ---------------------------------------------------------------------------
// Time. The simulator drives this explicitly rather than reading the wall
// clock, so a scenario can be stepped deterministically or run faster than
// real time. simSetMillis/simAdvance are defined in Arduino.cpp.
// ---------------------------------------------------------------------------
uint32_t millis();
uint32_t micros();
void     delay(uint32_t ms);
void     delayMicroseconds(uint32_t us);

void     simSetMillis(uint32_t ms);
void     simAdvanceMillis(uint32_t deltaMs);

// ---------------------------------------------------------------------------
// GPIO / ADC. No-ops on the host; the simulated HAL supplies input state.
// ---------------------------------------------------------------------------
void     pinMode(uint8_t pin, uint8_t mode);
void     digitalWrite(uint8_t pin, uint8_t value);
int      digitalRead(uint8_t pin);
int      analogRead(uint8_t pin);
void     analogReadResolution(uint8_t bits);

// ---------------------------------------------------------------------------
// Arduino String. Wraps std::string but must present the Arduino API, which
// the firmware uses heavily (toCharArray, String(float, decimals), operator+
// against numeric types, substring, ...).
// ---------------------------------------------------------------------------
class String {
public:
    String() {}
    String(const char* s) : _s(s ? s : "") {}
    String(const std::string& s) : _s(s) {}
    String(char c) : _s(1, c) {}
    String(int v, int base = DEC)                { _s = fromInt((long)v, base); }
    String(long v, int base = DEC)               { _s = fromInt(v, base); }
    String(unsigned int v, int base = DEC)       { _s = fromUInt((unsigned long)v, base); }
    String(unsigned long v, int base = DEC)      { _s = fromUInt(v, base); }
    String(double v, int decimals = 2)           { char b[64]; snprintf(b, sizeof(b), "%.*f", decimals, v); _s = b; }
    String(float v, int decimals = 2)            { char b[64]; snprintf(b, sizeof(b), "%.*f", decimals, (double)v); _s = b; }

    const char* c_str() const { return _s.c_str(); }
    unsigned int length() const { return (unsigned int)_s.size(); }
    char charAt(unsigned int i) const { return i < _s.size() ? _s[i] : '\0'; }
    char operator[](unsigned int i) const { return charAt(i); }

    // Matches Arduino semantics: copies at most size-1 characters and always
    // NUL-terminates. The firmware had a bug where the source length was
    // passed as size, so getting this right matters for the simulator to
    // reproduce (or not reproduce) that behaviour faithfully.
    void toCharArray(char* buf, unsigned int size) const {
        if (!buf || size == 0) return;
        unsigned int n = std::min<unsigned int>(size - 1, (unsigned int)_s.size());
        memcpy(buf, _s.data(), n);
        buf[n] = '\0';
    }

    String substring(unsigned int from) const {
        if (from >= _s.size()) return String();
        return String(_s.substr(from));
    }
    String substring(unsigned int from, unsigned int to) const {
        if (from >= _s.size() || to <= from) return String();
        return String(_s.substr(from, to - from));
    }
    void toUpperCase() { for (auto& c : _s) c = (char)toupper((unsigned char)c); }
    void toLowerCase() { for (auto& c : _s) c = (char)tolower((unsigned char)c); }
    int indexOf(char c) const { auto p = _s.find(c); return p == std::string::npos ? -1 : (int)p; }
    long toInt() const { return strtol(_s.c_str(), nullptr, 10); }
    float toFloat() const { return strtof(_s.c_str(), nullptr); }

    String& operator+=(const String& o) { _s += o._s; return *this; }
    String& operator+=(const char* o)   { _s += (o ? o : ""); return *this; }
    String& operator+=(char o)          { _s += o; return *this; }

    // ArduinoJson appends through concat().
    bool concat(const String& o) { _s += o._s; return true; }
    bool concat(const char* o)   { _s += (o ? o : ""); return true; }
    bool concat(char o)          { _s += o; return true; }

    bool operator==(const String& o) const { return _s == o._s; }
    bool operator!=(const String& o) const { return _s != o._s; }
    bool operator==(const char* o) const { return _s == (o ? o : ""); }
    bool operator!=(const char* o) const { return !(*this == o); }
    bool operator<(const String& o) const { return _s < o._s; }

    const std::string& str() const { return _s; }

private:
    std::string _s;

    static std::string fromInt(long v, int base) {
        char b[64];
        if (base == HEX) snprintf(b, sizeof(b), "%lX", (unsigned long)v);
        else if (base == BIN) {
            std::string r;
            unsigned long u = (unsigned long)v;
            if (!u) return "0";
            while (u) { r.insert(r.begin(), char('0' + (u & 1))); u >>= 1; }
            return r;
        }
        else snprintf(b, sizeof(b), "%ld", v);
        return b;
    }
    static std::string fromUInt(unsigned long v, int base) {
        char b[64];
        if (base == HEX) snprintf(b, sizeof(b), "%lX", v);
        else snprintf(b, sizeof(b), "%lu", v);
        return b;
    }
};

inline String operator+(const String& a, const String& b) { String r(a); r += b; return r; }
inline String operator+(const String& a, const char* b)   { String r(a); r += b; return r; }
inline String operator+(const char* a, const String& b)   { String r(a); r += b; return r; }
inline String operator+(const String& a, char b)          { String r(a); r += b; return r; }
inline String operator+(const String& a, int b)           { String r(a); r += String(b); return r; }
inline String operator+(const String& a, long b)          { String r(a); r += String(b); return r; }
inline String operator+(const String& a, unsigned int b)  { String r(a); r += String(b); return r; }
inline String operator+(const String& a, unsigned long b) { String r(a); r += String(b); return r; }
inline String operator+(const String& a, float b)         { String r(a); r += String(b); return r; }
inline String operator+(const String& a, double b)        { String r(a); r += String(b); return r; }

class Printable {
public:
    virtual ~Printable() {}
    virtual size_t printTo(class Print& p) const = 0;
};

#include "Print.h"

// Minimal Stream. ArduinoJson and TinyGPSPlus reference it; the firmware
// never uses one on the host.
class Stream : public Print {
public:
    virtual int available() { return 0; }
    virtual int read() { return -1; }
    virtual int peek() { return -1; }
    virtual size_t readBytes(char* buf, size_t len) { (void)buf; (void)len; return 0; }
    virtual size_t readBytes(uint8_t* buf, size_t len) { (void)buf; (void)len; return 0; }
    virtual void setTimeout(unsigned long) {}
    size_t write(uint8_t) override { return 0; }
};

// ---------------------------------------------------------------------------
// Serial. Writes to stdout so simulator runs are greppable, and so the
// firmware's diagnostic printf-debugging still tells you something.
// ---------------------------------------------------------------------------
class SerialStub : public Print {
public:
    void begin(unsigned long) {}
    void end() {}
    operator bool() const { return true; }
    size_t write(uint8_t c) override { fputc(c, stdout); return 1; }
    size_t write(const uint8_t* b, size_t n) override { fwrite(b, 1, n, stdout); return n; }
    int printf(const char* fmt, ...);
    void printBufferReverse(const uint8_t* buf, int len, char sep);
    void flush() { fflush(stdout); }
};

extern SerialStub Serial;

#endif /* ARDUINO_H_STUB */

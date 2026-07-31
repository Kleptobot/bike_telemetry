// Arduino Print base class. Adafruit_GFX derives from this, and the firmware
// calls print()/println() on Serial, on File32 and on the canvas.
#ifndef PRINT_H_STUB
#define PRINT_H_STUB

#include <cstdint>
#include <cstddef>
#include <cstdio>

class String;

class Print {
public:
    virtual ~Print() {}

    // The one method subclasses must provide.
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t* buffer, size_t size) {
        size_t n = 0;
        while (size--) n += write(*buffer++);
        return n;
    }
    size_t write(const char* s) {
        if (!s) return 0;
        return write((const uint8_t*)s, strlen(s));
    }
    size_t write(const char* buffer, size_t size) {
        return write((const uint8_t*)buffer, size);
    }

    size_t print(const char* s)      { return write(s); }
    size_t print(char c)             { return write((uint8_t)c); }
    size_t print(int v, int base = 10);
    size_t print(long v, int base = 10);
    size_t print(unsigned int v, int base = 10);
    size_t print(unsigned long v, int base = 10);
    size_t print(double v, int decimals = 2);
    size_t print(float v, int decimals = 2) { return print((double)v, decimals); }
    size_t print(const String& s);
    size_t print(const class Printable& p);
    // Disambiguates println(size_t) on 64-bit hosts, where size_t matches
    // neither unsigned int nor unsigned long unambiguously.
    size_t print(unsigned long long v, int base = 10);
    size_t println(unsigned long long v, int base = 10) { size_t n = print(v, base); return n + println(); }

    size_t println();
    size_t println(const char* s)    { size_t n = print(s); return n + println(); }
    size_t println(char c)           { size_t n = print(c); return n + println(); }
    size_t println(int v, int base = 10)           { size_t n = print(v, base); return n + println(); }
    size_t println(long v, int base = 10)          { size_t n = print(v, base); return n + println(); }
    size_t println(unsigned int v, int base = 10)  { size_t n = print(v, base); return n + println(); }
    size_t println(unsigned long v, int base = 10) { size_t n = print(v, base); return n + println(); }
    size_t println(double v, int decimals = 2)     { size_t n = print(v, decimals); return n + println(); }
    size_t println(float v, int decimals = 2)      { return println((double)v, decimals); }
    size_t println(const String& s);
};

#endif /* PRINT_H_STUB */

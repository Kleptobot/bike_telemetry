// Host PCF85063A stub: mirrors the real RTC_NXP API surface that
// src/HAL/Sensors.cpp uses, free-running off the simulated millis() clock
// exactly like the old RTClib DS3231 stub did.
#ifndef PCF85063A_H_STUB
#define PCF85063A_H_STUB

#include "Arduino.h"
#include <time.h>

class PCF85063A {
public:
    PCF85063A() {}

    void begin() {}

    // Register overwrite with bit-mask -- a no-op: there is no device.
    void bit_op8(uint8_t reg, uint8_t mask, uint8_t value) {
        (void)reg; (void)mask; (void)value;
    }

    // Set the calendar; the clock then free-runs off simulated millis(), so
    // the scenario's GPS-time resync (App -> HAL::setTime) behaves as it does
    // on device. mktime() matches the App's own GPS-time conversion, which
    // also goes through mktime.
    void set(struct tm* now_tm) {
        _baseUnix   = (uint32_t)mktime(now_tm);
        _baseMillis = millis();
    }

    // "time() in time.h compatible" accessor (RTC_NXP::time(time_t*)).
    time_t time(time_t* tp) {
        const uint32_t elapsed = (millis() - _baseMillis) / 1000U;
        const time_t t = (time_t)(_baseUnix + elapsed);
        if (tp) *tp = t;
        return t;
    }

private:
    // Unix seconds at the moment the RTC was last set, and the simulated
    // millis() value then, so time() advances with simulated time.
    static uint32_t _baseUnix;
    static uint32_t _baseMillis;
};

inline uint32_t PCF85063A::_baseUnix   = 1767225600UL;  // 2026-01-01T00:00:00Z, same seed as the old RTClib stub
inline uint32_t PCF85063A::_baseMillis = 0;

#endif /* PCF85063A_H_STUB */
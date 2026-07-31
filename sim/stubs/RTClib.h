// Host RTClib stub: DateTime/TimeSpan plus a simulated DS3231.
//
// DateTime is real arithmetic, not a placeholder -- timeData converts through
// it, the GPS time sync compares against it, and the SD timestamp callback
// reads it, so getting the calendar maths right here is load-bearing for the
// simulator to reproduce device behaviour.
#ifndef RTCLIB_H_STUB
#define RTCLIB_H_STUB

#include "Arduino.h"

#define SECONDS_FROM_1970_TO_2000 946684800UL

class TimeSpan {
public:
    TimeSpan(int32_t seconds = 0) : _s(seconds) {}
    TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds)
        : _s((int32_t)days * 86400L + (int32_t)hours * 3600L + (int32_t)minutes * 60L + seconds) {}
    int32_t totalseconds() const { return _s; }
    int16_t days() const { return (int16_t)(_s / 86400L); }
    int8_t  hours() const { return (int8_t)((_s % 86400L) / 3600L); }
    int8_t  minutes() const { return (int8_t)((_s % 3600L) / 60L); }
    int8_t  seconds() const { return (int8_t)(_s % 60L); }
private:
    int32_t _s;
};

class DateTime {
public:
    DateTime() : _y(2000), _mo(1), _d(1), _h(0), _mi(0), _s(0) {}
    DateTime(uint16_t y, uint8_t mo, uint8_t d, uint8_t h = 0, uint8_t mi = 0, uint8_t s = 0)
        : _y(y), _mo(mo), _d(d), _h(h), _mi(mi), _s(s) {}
    explicit DateTime(uint32_t secondsSince2000) { fromUnix((uint32_t)(secondsSince2000 + SECONDS_FROM_1970_TO_2000)); }

    uint16_t year() const { return _y; }
    uint8_t  month() const { return _mo; }
    uint8_t  day() const { return _d; }
    uint8_t  hour() const { return _h; }
    uint8_t  minute() const { return _mi; }
    uint8_t  second() const { return _s; }

    uint32_t unixtime() const {
        return (uint32_t)(daysFromCivil(_y, _mo, _d) * 86400LL
                          + (int64_t)_h * 3600 + (int64_t)_mi * 60 + _s);
    }
    uint32_t secondstime() const { return unixtime() - SECONDS_FROM_1970_TO_2000; }
    bool isValid() const { return _mo >= 1 && _mo <= 12 && _d >= 1 && _d <= 31 && _h < 24 && _mi < 60 && _s < 60; }

    TimeSpan operator-(const DateTime& o) const {
        return TimeSpan((int32_t)((int64_t)unixtime() - (int64_t)o.unixtime()));
    }
    DateTime operator+(const TimeSpan& ts) const {
        DateTime r; r.fromUnix((uint32_t)((int64_t)unixtime() + ts.totalseconds())); return r;
    }

private:
    uint16_t _y; uint8_t _mo, _d, _h, _mi, _s;

    // Howard Hinnant's days_from_civil, the same algorithm the firmware's
    // timeData now uses, so the two agree by construction.
    static int64_t daysFromCivil(int y, unsigned m, unsigned d) {
        y -= m <= 2;
        const int64_t era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = (unsigned)(y - era * 400);
        const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097LL + (int64_t)doe - 719468LL;
    }

    void fromUnix(uint32_t t) {
        int64_t days = (int64_t)(t / 86400U);
        uint32_t rem = t % 86400U;
        _h = (uint8_t)(rem / 3600); rem %= 3600;
        _mi = (uint8_t)(rem / 60);
        _s = (uint8_t)(rem % 60);

        // Inverse of days_from_civil.
        days += 719468;
        const int64_t era = (days >= 0 ? days : days - 146096) / 146097;
        const unsigned doe = (unsigned)(days - era * 146097);
        const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        const int64_t yr = (int64_t)yoe + era * 400;
        const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        const unsigned mp = (5 * doy + 2) / 153;
        _d = (uint8_t)(doy - (153 * mp + 2) / 5 + 1);
        _mo = (uint8_t)(mp + (mp < 10 ? 3 : -9));
        _y = (uint16_t)(yr + (_mo <= 2));
    }
};

/** Simulated DS3231. Free-runs off the simulated millis() clock. */
class RTC_DS3231 {
public:
    bool begin() { return true; }
    void adjust(const DateTime& dt);
    DateTime now() const;
    float getTemperature() const { return _tempC; }
    bool lostPower() const { return false; }

    // Simulator hooks.
    void simSetTemperature(float c) { _tempC = c; }

private:
    // Unix seconds at the moment the RTC was last set, and the simulated
    // millis() value then, so now() advances with simulated time.
    static uint32_t _baseUnix;
    static uint32_t _baseMillis;
    static float    _tempC;
};

typedef RTC_DS3231 RTC_PCF8563;

#endif /* RTCLIB_H_STUB */

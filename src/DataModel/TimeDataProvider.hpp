#pragma once
#include <Arduino.h>
#include <RTClib.h>

struct timeDuration {
    long _totalSeconds;
 
    explicit timeDuration(long totalSeconds = 0) : _totalSeconds(totalSeconds) {}
    timeDuration(int days, int hours, int minutes, int seconds)
        : _totalSeconds(((long)days*24L + hours)*3600L + (long)minutes*60L + seconds) {}
 
    long totalSeconds() const { return _totalSeconds; }
    long days()    const { return _totalSeconds / 86400L; }
    long totalHours() const { return _totalSeconds / 3600L; } // raw, unwrapped (e.g. 27 for a 27h lap)
    int  hours()   const { return (int)((_totalSeconds % 86400L) / 3600L); }
    int  minutes() const { return (int)((_totalSeconds % 3600L) / 60L); }
    int  seconds() const { return (int)(_totalSeconds % 60L); }
 
    timeDuration operator-() const { return timeDuration(-_totalSeconds); }
};

struct timeData {
private:
    friend class TimeDataProvider;

    // Internal storage is ALWAYS true UTC.
    int _year, _month, _day, _hour, _minute, _second;
    int _offset; // minutes, e.g. +600 for UTC+10

    static bool is_leap_year_s(int y) {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }

    static int get_days_in_month_s(int m, int y) {
        const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m == 2 && is_leap_year_s(y)) {
            return 29;
        }
        return days[m - 1];
    }

    bool is_leap_year(int y) const { return is_leap_year_s(y); }
    int get_days_in_month(int m, int y) const { return get_days_in_month_s(m, y); }

    // Normalizes an arbitrary (possibly out-of-range) y/m/d/h/min/s tuple in place.
    // Static so it can be reused for both the UTC fields and scratch local copies,
    // without needing a temporary timeData object.
    static void normalize_fields(int& y, int& mo, int& d, int& h, int& min, int& s) {
        // 1. Balance Seconds
        if (s >= 60) { min += s / 60; s %= 60; }
        else if (s < 0) { int borrow = (abs(s) + 59) / 60; min -= borrow; s += borrow * 60; }

        // 2. Balance Minutes
        if (min >= 60) { h += min / 60; min %= 60; }
        else if (min < 0) { int borrow = (abs(min) + 59) / 60; h -= borrow; min += borrow * 60; }

        // 3. Balance Hours
        if (h >= 24) { d += h / 24; h %= 24; }
        else if (h < 0) { int borrow = (abs(h) + 23) / 24; d -= borrow; h += borrow * 24; }

        // 4. Balance Days & Months (Handles positive and negative jumps)
        while (d > get_days_in_month_s(mo, y)) {
            d -= get_days_in_month_s(mo, y);
            mo++;
            if (mo > 12) { mo = 1; y++; }
        }
        while (d <= 0) {
            mo--;
            if (mo < 1) { mo = 12; y--; }
            d += get_days_in_month_s(mo, y);
        }
    }

    void normalize_time() {
        normalize_fields(_year, _month, _day, _hour, _minute, _second);
    }

    // Computes the local-time view (UTC fields shifted by _offset minutes),
    // without mutating stored UTC fields.
    void to_local(int& y, int& mo, int& d, int& h, int& min, int& s) const {
        y = _year; mo = _month; d = _day; h = _hour; min = _minute; s = _second;
        min += _offset;
        normalize_fields(y, mo, d, h, min, s);
    }

    // Inverse: given a desired local tuple, computes the corresponding UTC tuple
    // (subtract offset, normalize). Used by the add_* methods, which operate in
    // local terms but must store UTC.
    void from_local(int y, int mo, int d, int h, int min, int s) {
        min -= _offset;
        normalize_fields(y, mo, d, h, min, s);
        _year = y; _month = mo; _day = d; _hour = h; _minute = min; _second = s;
    }
 
    // Days elapsed from a fixed epoch (year 0, day 1 of month 1) to the given
    // date. Used to compute a single linear "total seconds since epoch" value
    // for operator- / operator+, so calendar differences reduce to plain
    // integer math without duplicating leap-year handling.
    static long days_since_epoch_s(int y, int mo, int d) {
        long days = d - 1;
        for (int m = 1; m < mo; ++m) days += get_days_in_month_s(m, y);
        for (int yr = 0; yr < y; ++yr) days += is_leap_year_s(yr) ? 366 : 365;
        return days;
    }
 
    long utc_total_seconds() const {
        return days_since_epoch_s(_year, _month, _day) * 86400L
             + (long)_hour * 3600L + (long)_minute * 60L + _second;
    }

    void setOffset(int offset_) { _offset = offset_; }

public:
    timeData(){}
    timeData(int y, int m, int d, int h, int min, int s, int offset = 0)
        : _year(y), _month(m), _day(d), _hour(h), _minute(min), _second(s), _offset(offset) {
        normalize_time();
    }

    timeData(DateTime dt, int offset = 0) : _offset(offset){
        _year = dt.year();
        _month = dt.month();
        _day = dt.day();
        _hour = dt.hour();
        _minute = dt.minute();
        _second = dt.second();
        normalize_time();
    }

    timeData(long long unixtime, int offset = 0) : _offset(offset) {
        int y = 1970, mo = 1, d = 1, h = 0, mi = 0, s = 0;
        long long totalSeconds = unixtime;

        s = (int)(totalSeconds % 60); totalSeconds /= 60;
        mi = (int)(totalSeconds % 60); totalSeconds /= 60;
        h = (int)(totalSeconds % 24); totalSeconds /= 24;
        d += (long)totalSeconds; // remaining totalSeconds is now whole days

        normalize_fields(y, mo, d, h, mi, s);

        _year = y; _month = mo; _day = d;
        _hour = h; _minute = mi; _second = s;
    }

    // Seconds since 1970-01-01T00:00:00 UTC (the Unix epoch), computed from the
    // true UTC fields - offset-independent, matching utc_total_seconds()'s epoch-blind
    // day-counting approach. Returns a signed/wide type since dates before 1970
    // are representable here (unlike RTClib's DateTime) and would otherwise underflow
    // an unsigned return type.
    long long unixtime() const {
        static const long EPOCH_DAYS = days_since_epoch_s(1970, 1, 1);
        return (days_since_epoch_s(_year, _month, _day) - EPOCH_DAYS) * 86400LL
            + (long long)_hour * 3600LL + (long long)_minute * 60LL + _second;
    }

    // --- Local-time getters (offset-adjusted) ---
    int year()   const { int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s); return y;  }
    int month()  const { int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s); return mo; }
    int day()    const { int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s); return d;  }
    int hour()   const { int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s); return h;  }
    int minute() const { int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s); return mi; }
    int second() const { int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s); return s;  }
    int offset() const { return _offset; }

    struct localTimeView {
        int year, month, day, hour, minute, second;
    };
    localTimeView local() const {
        localTimeView v;
        to_local(v.year, v.month, v.day, v.hour, v.minute, v.second);
        return v;
    }

    // --- True UTC getters (for RTC writes / anything that must bypass offset) ---
    int utcYear()   const { return _year; }
    int utcMonth()  const { return _month; }
    int utcDay()    const { return _day; }
    int utcHour()   const { return _hour; }
    int utcMinute() const { return _minute; }
    int utcSecond() const { return _second; }

    // Returns a DateTime built from true UTC fields - the correct thing to
    // hand to an RTC chip (which has no concept of offset).
    DateTime utcDateTime() const {
        return {_year, _month, _day, _hour, _minute, _second};
    }

    // --- Unified Delta Methods ---
    // amount is interpreted in LOCAL terms: e.g. add_days(1) advances the
    // calendar day as seen in local time, which may not be the same instant-
    // -relative change as adding 1 to the stored UTC day when offset pushes
    // the local time across a day boundary the UTC fields haven't crossed yet.
    void add_offset(int amount)  {
        _offset += amount;
        _offset = constrain(_offset, -13*60, 13*60);
    }

    void add_seconds(int amount) {
        int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s);
        s += amount;
        from_local(y,mo,d,h,mi,s);
    }
    void add_minutes(int amount) {
        int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s);
        mi += amount;
        from_local(y,mo,d,h,mi,s);
    }
    void add_hours(int amount) {
        int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s);
        h += amount;
        from_local(y,mo,d,h,mi,s);
    }
    void add_days(int amount) {
        int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s);
        d += amount;
        from_local(y,mo,d,h,mi,s);
    }
    void add_months(int amount) {
        int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s);
        mo += amount;
        while (mo > 12) { mo -= 12; y++; }
        while (mo < 1)  { mo += 12; y--; }
        from_local(y,mo,d,h,mi,s);
    }
    void add_years(int amount) {
        int y,mo,d,h,mi,s; to_local(y,mo,d,h,mi,s);
        y += amount;
        from_local(y,mo,d,h,mi,s);
    }

    timeData& operator=(const timeData& _new) {
        if (this == &_new) {
            return *this;
        }
        _year = _new._year;
        _month = _new._month;
        _day = _new._day;
        _hour = _new._hour;
        _minute = _new._minute;
        _second = _new._second;
        _offset = _new._offset;

        return *this;
    }

    // --- Arithmetic operators ---
    // Adds an elapsed duration to this instant. Operates on UTC fields
    // directly: shifting an instant by N seconds is the same shift
    // regardless of offset, so no local/UTC conversion is needed here
    // (unlike add_days/add_months, which have calendar-boundary semantics).
    timeData operator+(const timeDuration& dur) const {
        int y = _year, mo = _month, d = _day, h = _hour, mi = _minute, s = _second;
        s += (int)(dur.totalSeconds() % 60L);
        long carryMinutes = dur.totalSeconds() / 60L;
        mi += (int)(carryMinutes % 60L);
        long carryHours = carryMinutes / 60L;
        h += (int)(carryHours % 24L);
        d += (int)(carryHours / 24L);
        normalize_fields(y, mo, d, h, mi, s);
        timeData result;
        result._year = y; result._month = mo; result._day = d;
        result._hour = h; result._minute = mi; result._second = s;
        result._offset = _offset;
        return result;
    }
 
    timeData operator-(const timeDuration& dur) const {
        return *this + timeDuration(-dur.totalSeconds());
    }
 
    // Difference between two instants, in absolute (offset-independent) terms.
    // Two timeData values representing the same UTC instant - even with
    // different _offset - yield a zero duration.
    timeDuration operator-(const timeData& rhs) const {
        return timeDuration(this->utc_total_seconds() - rhs.utc_total_seconds());
    }
};

class TimeDataProvider {
public:
    const timeData& get() const { return _data; }
    timeData& get() { return _data; } // For mutable access
    uint32_t version() const { return _version; }

    void setUTCOffset(int UTCoffset) { 
        _data.setOffset(UTCoffset);
        ++_version;
    }

    void setDate(timeData _date) {
        _data = _date;
        ++_version;
    }

    void update(const timeData& newData) {
        _data = newData;
        ++_version;
    }

private:
    timeData _data{};
    uint32_t _version = 0;
};
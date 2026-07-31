#include "RTClib.h"

uint32_t RTC_DS3231::_baseUnix   = 1767225600UL;  // 2026-01-01T00:00:00Z, arbitrary but sane
uint32_t RTC_DS3231::_baseMillis = 0;
float    RTC_DS3231::_tempC      = 21.5f;

void RTC_DS3231::adjust(const DateTime& dt) {
    _baseUnix   = dt.unixtime();
    _baseMillis = millis();
}

DateTime RTC_DS3231::now() const {
    // Free-runs off simulated millis(), so the clock advances with the
    // scenario rather than the wall clock.
    const uint32_t elapsed = (millis() - _baseMillis) / 1000U;
    DateTime d;
    d = DateTime((uint32_t)(_baseUnix + elapsed - SECONDS_FROM_1970_TO_2000));
    return d;
}

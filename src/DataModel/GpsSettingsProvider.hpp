#ifndef GPS_SETTINGS_PROVIDER_H
#define GPS_SETTINGS_PROVIDER_H

#include <stdint.h>

static constexpr uint8_t GPS_NMEA_RATE_COUNT = 10;

struct GpsSettings {
    uint8_t nmeaRates[GPS_NMEA_RATE_COUNT] = {1, 0, 0, 0, 1, 0, 0, 0, 0, 0};
};

class GpsSettingsProvider {
public:
    const GpsSettings& get() const { return _data; }
    uint32_t version() const { return _version; }

    void update(const GpsSettings& newData) {
        _data = newData;
        ++_version;
    }

private:
    GpsSettings _data{};
    uint32_t _version = 0;
};

#endif /* GPS_SETTINGS_PROVIDER_H */

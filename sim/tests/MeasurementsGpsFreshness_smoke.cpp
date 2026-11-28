// Standalone smoke test for GNSS sample freshness. Build and run from sim/:
//   g++ -std=gnu++17 -I stubs -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\TinyGPSPlus\src tests\MeasurementsGpsFreshness_smoke.cpp ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\TinyGPSPlus\src\TinyGPS++.cpp stubs\Arduino.cpp -o build\MeasurementsGpsFreshness_smoke.exe
//   build\MeasurementsGpsFreshness_smoke.exe

#include "HAL/Measurements.hpp"

#include <cstdio>

void simSetMillis(uint32_t ms);

static void feed(TinyGPSPlus& gps, const char* body) {
    uint8_t checksum = 0;
    for (const char* p = body; *p; ++p) checksum ^= static_cast<uint8_t>(*p);

    char sentence[160];
    std::snprintf(sentence, sizeof(sentence), "$%s*%02X\r\n", body, checksum);
    for (const char* p = sentence; *p; ++p) gps.encode(*p);
}

static void feedFix(TinyGPSPlus& gps) {
    feed(gps, "GPRMC,120000.00,A,4807.038,N,01131.000,E,10.00,0.00,010126,,,A");
    feed(gps, "GPGGA,120000.00,4807.038,N,01131.000,E,1,09,0.9,545.4,M,46.9,M,,");
}

int main() {
    TinyGPSPlus gps;
    MeasurementFrame frame{};
    GpsTrackers trackers{};

    simSetMillis(1000);
    feedFix(gps);
    fillGps(frame, gps, millis(), trackers);
    if (!frame.gpsPos.valid || !frame.gpsSpeedKmh.valid ||
        !frame.gpsAltM.valid || !frame.gpsHdop.valid ||
        !frame.gpsSatsUsed.valid) {
        std::printf("FAIL: newly committed GNSS samples were not valid\n");
        return 1;
    }

    simSetMillis(1000 + GPS_SAMPLE_STALE_TIMEOUT_MS);
    fillGps(frame, gps, millis(), trackers);
    if (!frame.gpsPos.valid || !frame.gpsSpeedKmh.valid) {
        std::printf("FAIL: GNSS samples expired at the timeout boundary\n");
        return 1;
    }

    simSetMillis(1000 + GPS_SAMPLE_STALE_TIMEOUT_MS + 1);
    fillGps(frame, gps, millis(), trackers);
    if (frame.gpsPos.valid || frame.gpsSpeedKmh.valid ||
        frame.gpsAltM.valid || frame.gpsHdop.valid ||
        frame.gpsSatsUsed.valid || frame.gpsPos.tsMs != 0 ||
        frame.gpsSpeedKmh.tsMs != 0) {
        std::printf("FAIL: stale GNSS samples remained valid\n");
        return 1;
    }

    simSetMillis(5000);
    feedFix(gps);
    fillGps(frame, gps, millis(), trackers);
    if (!frame.gpsPos.valid || !frame.gpsSpeedKmh.valid ||
        !frame.gpsAltM.valid || !frame.gpsHdop.valid ||
        !frame.gpsSatsUsed.valid) {
        std::printf("FAIL: fresh GNSS samples did not restore validity\n");
        return 1;
    }

    std::printf("PASS: GNSS samples expire after the freshness timeout and recover on a new fix\n");
    return 0;
}

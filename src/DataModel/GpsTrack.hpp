#ifndef GPSTRACK_H
#define GPSTRACK_H

#include <Arduino.h>
#include <vector>

// Breadcrumb point for the map's recent-track polyline.
struct GPSPoint {
    double lat;
    double lon;
    uint32_t ts;
    GPSPoint() : lat(0), lon(0), ts(0) {}
    GPSPoint(double la, double lo, uint32_t t) : lat(la), lon(lo), ts(t) {}
};

// Decimated recent-position store backing the map widget's breadcrumb trail.
// Extracted verbatim from TelemetryDataProvider (which also used to carry the
// Telemetry struct): append() decimates by time -- GPS fixes arrive at 1 Hz,
// so anything faster duplicates the same fix -- and rejects identical
// consecutive positions. Consumers rely on points() being chronological.
class GpsTrack {
public:
    void append(double lat, double lon, uint32_t ts) {
        if (!_recent.empty() && (ts - _lastPointMs) < _minPointIntervalMs) return;

        if (!_recent.empty() && _recent.back().lat == lat && _recent.back().lon == lon) return;

        _recent.emplace_back(lat, lon, ts);
        _lastPointMs = ts;
        ++_version;

        // erase(begin()) is O(n), but with the decimation above it runs at
        // most once per second over 600 elements. A ring buffer would avoid
        // the memmove, but consumers rely on _recent being in chronological
        // order -- MapWidget draws the polyline by iterating it and takes
        // back() as the current position -- so it is not worth the ordering
        // hazard for a 14 KB move at 1 Hz.
        if (_recent.size() > _maxPoints) {
            _recent.erase(_recent.begin());
        }
    }

    const std::vector<GPSPoint>& points() const { return _recent; }
    void clear() { _recent.clear(); ++_version; }
    uint32_t version() const { return _version; }

private:
    std::vector<GPSPoint> _recent;
    uint32_t _lastPointMs = 0;
    uint32_t _version = 0;
    static const size_t _maxPoints = 600;
    static const uint32_t _minPointIntervalMs = 1000;   // GPS fixes arrive at 1 Hz
};

#endif /* GPSTRACK_H */

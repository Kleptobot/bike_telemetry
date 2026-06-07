#pragma once
#include "UI/Widgets/Widget.hpp"
#include "display/Display.hpp"
#include "DataModel/DataModel.hpp"

class MapWidget : public Widget {
public:
    MapWidget(int x, int y, int w, int h, DataModel& model);

    void update(float dt) override;
    void render() override;

    void setZoom(float metersPerPixel) { _metersPerPixel = metersPerPixel; _tilesNeedReload = true; invalidate(); }
    void setCenter(double lat, double lon) { _centerLat = lat; _centerLon = lon; invalidate(); }
    static constexpr int MAP_W = 80;
    static constexpr int MAP_H = 80;

private:
    // --- Layout constants ---

    // --- Tile cache ---
    // Composited tile pixels for the current viewport.
    // 160 * 160 * 2 = 51,200 bytes, allocated once for the life of the program.
    //uint16_t _tilePixels[MAP_W * MAP_H];
    // static uint16_t tilePixels[MapWidget::MAP_W * MapWidget::MAP_H];
    bool _tilesCached = false;

    // Tracks which tiles are currently composited into _tilePixels so we
    // only reload from SD when the visible set actually changes.
    struct TileKey {
        int zoom = -1;
        uint32_t tx = 0;
        uint32_t ty = 0;

        bool operator==(const TileKey& o) const {
            return zoom == o.zoom && tx == o.tx && ty == o.ty;
        }
    };
    static constexpr int MAX_VISIBLE_TILES = 4; // 2x2 grid max
    TileKey _cachedTileKeys[MAX_VISIBLE_TILES];
    int _cachedTileCount = 0;

    // --- State ---
    DataModel& _model;
    double _centerLat  = 0.0;
    double _centerLon  = 0.0;
    float  _metersPerPixel = 4.0f;
    int    _currentZoom    = -1;   // cached, recomputed only when _metersPerPixel changes
    uint32_t _lastVersion  = 0;
    bool   _dirty          = true;
    bool   _tilesNeedReload = true;

    // --- Private methods ---

    // Recompute _currentZoom from _metersPerPixel and _centerLat.
    void computeZoom();

    // Fill keys[]/count with the set of tile coords visible in the current viewport.
    void computeVisibleTiles(TileKey* keys, int& count) const;

    // Return true if keys[]/count differs from _cachedTileKeys/_cachedTileCount.
    bool visibleTilesChanged(const TileKey* keys, int count) const;

    // Load all visible tiles from SD into _tilePixels.
    // This is the ONLY place SD card access happens in this widget.
    void loadTilesToCache();

    // Blit _tilePixels into the canvas. No SD access.
    void renderFromCache();

    // Draw the breadcrumb track and current-position dot over whatever is on canvas.
    void renderTrack();

    // Project a lat/lon to widget-local pixel coordinates.
    void project(double lat, double lon, int& outX, int& outY) const;
};
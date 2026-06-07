#include "MapWidget.hpp"
#include <cmath>
#include "Display/Display.hpp"
#include "Map/TileLoader.hpp"
static uint16_t tilePixels[MapWidget::MAP_W * MapWidget::MAP_H];


// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
MapWidget::MapWidget(int x, int y, int w, int h, DataModel& model)
    : Widget(x, y, w, h), _model(model) {
    // _tilePixels is a plain array member — no heap allocation needed.
    memset(tilePixels, 0, sizeof(tilePixels));
}


// ---------------------------------------------------------------------------
// update() — runs every tick. All SD access is triggered from here.
// render() does zero SD access.
// ---------------------------------------------------------------------------

void MapWidget::update(float dt) {
    // If the widget is not on screen, do nothing. This prevents SD loads
    // when the user is on the telemetry screen.
    if (!visible) return;

    const auto& telem = _model.telemetry();
    if (telem.version() == _lastVersion) return;
    _lastVersion = telem.version();

    const auto& t = telem.get();
    if (t.validLocation) {
        _centerLat = t.latitude;
        _centerLon = t.longitude;
    }

    // Recompute zoom (cheap, only recalculates when _metersPerPixel changes).
    computeZoom();

    // Check whether the set of visible tiles has changed since last load.
    TileKey newKeys[MAX_VISIBLE_TILES];
    int newCount = 0;
    computeVisibleTiles(newKeys, newCount);

    if (_tilesNeedReload || !_tilesCached || visibleTilesChanged(newKeys, newCount)) {
        loadTilesToCache(); // <-- only SD access point in this widget
        memcpy(_cachedTileKeys, newKeys, sizeof(TileKey) * newCount);
        _cachedTileCount = newCount;
        _tilesCached     = true;
        _tilesNeedReload = false;
    }

    _dirty = true;
}

// ---------------------------------------------------------------------------
// render() — no SD access. Composites cached tiles then draws track overlay.
// ---------------------------------------------------------------------------

void MapWidget::render() {
    if (!visible || !_dirty) return;

    // Clear widget area.
    Disp::fillRect(x, y, _width, _height, ST77XX_BLACK);

    if (_tilesCached) {
        renderFromCache();
    } else {
        // No tiles loaded yet — show placeholder.
        Disp::drawText(x + 4, y + 4, String("No map data"), ST77XX_WHITE);
    }

    // Track overlay is drawn regardless of whether tiles are available,
    // so the breadcrumb trail shows even in the fallback state.
    renderTrack();

    Disp::markDirty(x, y, _width, _height);
    _dirty = false;
}

// ---------------------------------------------------------------------------
// computeZoom
// ---------------------------------------------------------------------------

void MapWidget::computeZoom() {
    double latRad = _centerLat * M_PI / 180.0;
    double metersPerPixelAtZoom0 = 156543.03392 * cos(latRad);
    double ideal = metersPerPixelAtZoom0 / _metersPerPixel;
    int zoom = (ideal > 1.0) ? (int)round(log2(ideal)) : 0;
    zoom = max(0, min(19, zoom));
    _currentZoom = zoom;
}

// ---------------------------------------------------------------------------
// computeVisibleTiles
// Determines which (zoom, tx, ty) tiles overlap the current widget viewport.
// ---------------------------------------------------------------------------

void MapWidget::computeVisibleTiles(TileKey* keys, int& count) const {
    count = 0;
    if (_currentZoom < 0) return;

    uint32_t centerTileX, centerTileY;
    TileLoader::latLonToTileXY(_centerLat, _centerLon, _currentZoom, centerTileX, centerTileY);

    int centerPixelXInTile, centerPixelYInTile;
    TileLoader::latLonToTilePixel(_centerLat, _centerLon, _currentZoom, centerPixelXInTile, centerPixelYInTile);

    int64_t globalCenterX = (int64_t)centerTileX * TileLoader::TILE_SIZE + centerPixelXInTile;
    int64_t globalCenterY = (int64_t)centerTileY * TileLoader::TILE_SIZE + centerPixelYInTile;

    int64_t widgetStartX = globalCenterX - MAP_W / 2;
    int64_t widgetStartY = globalCenterY - MAP_H / 2;
    int64_t widgetEndX   = widgetStartX + MAP_W;
    int64_t widgetEndY   = widgetStartY + MAP_H;

    int64_t tileX0 = widgetStartX / TileLoader::TILE_SIZE;
    if (widgetStartX < 0 && (widgetStartX % TileLoader::TILE_SIZE)) --tileX0;
    int64_t tileY0 = widgetStartY / TileLoader::TILE_SIZE;
    if (widgetStartY < 0 && (widgetStartY % TileLoader::TILE_SIZE)) --tileY0;
    int64_t tileX1 = (widgetEndX - 1) / TileLoader::TILE_SIZE;
    int64_t tileY1 = (widgetEndY - 1) / TileLoader::TILE_SIZE;

    for (int64_t ty = tileY0; ty <= tileY1 && count < MAX_VISIBLE_TILES; ++ty) {
        for (int64_t tx = tileX0; tx <= tileX1 && count < MAX_VISIBLE_TILES; ++tx) {
            keys[count++] = { _currentZoom, (uint32_t)tx, (uint32_t)ty };
        }
    }
}

// ---------------------------------------------------------------------------
// visibleTilesChanged
// ---------------------------------------------------------------------------

bool MapWidget::visibleTilesChanged(const TileKey* keys, int count) const {
    if (count != _cachedTileCount) return true;
    for (int i = 0; i < count; ++i) {
        if (!(keys[i] == _cachedTileKeys[i])) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// loadTilesToCache
// Reads tile data from SD into _tilePixels. This is the only function in
// MapWidget that performs SD card access.
// ---------------------------------------------------------------------------

void MapWidget::loadTilesToCache() {
    // Clear to black before compositing tiles.
    memset(tilePixels, 0, sizeof(tilePixels));

    if (_currentZoom < 0) return;

    uint32_t centerTileX, centerTileY;
    TileLoader::latLonToTileXY(_centerLat, _centerLon, _currentZoom, centerTileX, centerTileY);

    int centerPixelXInTile, centerPixelYInTile;
    TileLoader::latLonToTilePixel(_centerLat, _centerLon, _currentZoom, centerPixelXInTile, centerPixelYInTile);

    int64_t globalCenterX = (int64_t)centerTileX * TileLoader::TILE_SIZE + centerPixelXInTile;
    int64_t globalCenterY = (int64_t)centerTileY * TileLoader::TILE_SIZE + centerPixelYInTile;

    int64_t widgetStartX = globalCenterX - MAP_W / 2;
    int64_t widgetStartY = globalCenterY - MAP_H / 2;
    int64_t widgetEndX   = widgetStartX + MAP_W;
    int64_t widgetEndY   = widgetStartY + MAP_H;

    int64_t tileX0 = widgetStartX / TileLoader::TILE_SIZE;
    if (widgetStartX < 0 && (widgetStartX % TileLoader::TILE_SIZE)) --tileX0;
    int64_t tileY0 = widgetStartY / TileLoader::TILE_SIZE;
    if (widgetStartY < 0 && (widgetStartY % TileLoader::TILE_SIZE)) --tileY0;
    int64_t tileX1 = (widgetEndX - 1) / TileLoader::TILE_SIZE;
    int64_t tileY1 = (widgetEndY - 1) / TileLoader::TILE_SIZE;

    for (int64_t ty = tileY0; ty <= tileY1; ++ty) {
        for (int64_t tx = tileX0; tx <= tileX1; ++tx) {
            int64_t tileLeft = tx * TileLoader::TILE_SIZE;
            int64_t tileTop  = ty * TileLoader::TILE_SIZE;

            // Region of this tile that overlaps the widget viewport (global coords).
            int64_t copyStartX = max(tileLeft, widgetStartX);
            int64_t copyStartY = max(tileTop,  widgetStartY);
            int64_t copyEndX   = min(tileLeft + TileLoader::TILE_SIZE, widgetEndX);
            int64_t copyEndY   = min(tileTop  + TileLoader::TILE_SIZE, widgetEndY);

            int copyW = (int)(copyEndX - copyStartX);
            int copyH = (int)(copyEndY - copyStartY);
            if (copyW <= 0 || copyH <= 0) continue;

            // Source pixel offset within the tile.
            int srcX = (int)(copyStartX - tileLeft);
            int srcY = (int)(copyStartY - tileTop);

            // Destination offset within _tilePixels.
            int destX = (int)(copyStartX - widgetStartX);
            int destY = (int)(copyStartY - widgetStartY);

            uint32_t tileUx = (uint32_t)tx;
            uint32_t tileUy = (uint32_t)ty;

            if (!TileLoader::tileExists(_currentZoom, tileUx, tileUy)) continue;

            // Load directly into _tilePixels rather than the global canvas.
            TileLoader::loadRawTileToBuffer(
                _currentZoom, tileUx, tileUy,
                tilePixels, MAP_W,   // dest buffer and its stride
                destX, destY,
                srcX, srcY,
                copyW, copyH
            );
        }
    }
}

// ---------------------------------------------------------------------------
// renderFromCache
// Blits _tilePixels into the canvas. No SD access.
// ---------------------------------------------------------------------------

void MapWidget::renderFromCache() {
    uint16_t* canvasBuf = canvas.getBuffer();

    for (int row = 0; row < MAP_H; ++row) {
        // Source row in _tilePixels.
        const uint16_t* src = tilePixels + row * MAP_W;

        // Destination row in the full-screen canvas.
        uint16_t* dst = canvasBuf + (y + row) * SCREEN_WIDTH + x;

        memcpy(dst, src, MAP_W * sizeof(uint16_t));
    }
}

// ---------------------------------------------------------------------------
// renderTrack
// Draws the breadcrumb polyline and current-position dot over the canvas.
// Called after renderFromCache() (or the fallback fill) so it always appears
// on top. No SD access.
// ---------------------------------------------------------------------------

void MapWidget::renderTrack() {
    const auto& track = _model.telemetry().recentTrack();
    if (track.empty()) {
        if (!_tilesCached) {
            // Nothing to show at all — display a message.
            Disp::drawText(x + 4, y + 20, String("No GPS"), ST77XX_WHITE);
        }
        return;
    }

    int px = 0, py = 0;
    bool first = true;
    for (const auto& p : track) {
        int sx, sy;
        project(p.lat, p.lon, sx, sy);

        // Clip to widget bounds before drawing.
        bool inBounds = (sx >= x && sx < x + _width && sy >= y && sy < y + _height);

        if (!first && inBounds) {
            canvas.drawLine(px, py, sx, sy, ST77XX_GREEN);
        }
        px = sx;
        py = sy;
        first = false;
    }

    // Current position dot.
    const auto& last = track.back();
    int lx, ly;
    project(last.lat, last.lon, lx, ly);
    canvas.fillCircle(lx, ly, 3, ST77XX_RED);
}

// ---------------------------------------------------------------------------
// project
// Equirectangular projection from lat/lon to widget-local pixel coordinates.
// ---------------------------------------------------------------------------

void MapWidget::project(double lat, double lon, int& outX, int& outY) const {
    const double metersPerDegLat = 111132.0;
    const double metersPerDegLon = 111320.0 * cos(_centerLat * M_PI / 180.0);

    double dxMeters = (lon - _centerLon) * metersPerDegLon;
    double dyMeters = (lat - _centerLat) * metersPerDegLat;

    double cx = x + (_width  / 2.0);
    double cy = y + (_height / 2.0);

    outX = (int)(cx + (dxMeters / _metersPerPixel));
    outY = (int)(cy - (dyMeters / _metersPerPixel));
}
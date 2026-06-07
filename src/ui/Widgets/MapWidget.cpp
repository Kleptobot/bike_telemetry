#include "MapWidget.hpp"
#include <cmath>
#include "Display/Display.hpp"
#include "Map/TileLoader.hpp"

MapWidget::MapWidget(int x, int y, int w, int h, DataModel& model)
    : Widget(x, y, w, h), _model(model) {
    _width = w;
    _height = h;
}

void MapWidget::update(float dt) {
    const auto& telem = _model.telemetry();
    if (telem.version() != _lastVersion) {
        _lastVersion = telem.version();
        // if there is a valid fix, center on it
        const auto& t = telem.get();
        if (t.validLocation) {
            _centerLat = t.latitude;
            _centerLon = t.longitude;
        }
        _dirty = true;
    }
}

void MapWidget::project(double lat, double lon, int& outX, int& outY) {
    // Equirectangular approx: meters per degree
    const double metersPerDegLat = 111132.0; // approximate
    const double metersPerDegLon = 111320.0 * cos(_centerLat * M_PI / 180.0);

    double dxMeters = (lon - _centerLon) * metersPerDegLon;
    double dyMeters = (lat - _centerLat) * metersPerDegLat;

    double cx = x + (_width / 2.0);
    double cy = y + (_height / 2.0);

    outX = int(cx + (dxMeters / _metersPerPixel));
    outY = int(cy - (dyMeters / _metersPerPixel));
}

void MapWidget::render() {
    if (!visible) return;

    if (!_dirty) return;

    // draw background box
    Disp::fillRect(x, y, _width, _height, ST77XX_BLACK);

    const auto& track = _model.telemetry().recentTrack();

    // Try to load map tiles first. Compute zoom from metersPerPixel.
    bool tilesLoaded = false;
    if (!track.empty()) {
        double latRad = _centerLat * M_PI / 180.0;
        double metersPerPixelAtZoom0 = 156543.03392 * cos(latRad);
        double ideal = metersPerPixelAtZoom0 / _metersPerPixel;
        int zoom = 0;
        if (ideal > 1.0) zoom = (int)round(log2(ideal));
        if (zoom < 0) zoom = 0;
        if (zoom > 19) zoom = 19;

        uint32_t centerTileX, centerTileY;
        TileLoader::latLonToTileXY(_centerLat, _centerLon, zoom, centerTileX, centerTileY);
        int centerPixelXInTile, centerPixelYInTile;
        TileLoader::latLonToTilePixel(_centerLat, _centerLon, zoom, centerPixelXInTile, centerPixelYInTile);

        int64_t globalCenterX = (int64_t)centerTileX * TileLoader::TILE_SIZE + centerPixelXInTile;
        int64_t globalCenterY = (int64_t)centerTileY * TileLoader::TILE_SIZE + centerPixelYInTile;

        int64_t widgetStartX = globalCenterX - _width / 2;
        int64_t widgetStartY = globalCenterY - _height / 2;
        int64_t widgetEndX = widgetStartX + _width;
        int64_t widgetEndY = widgetStartY + _height;

        int64_t tileX0 = widgetStartX / TileLoader::TILE_SIZE;
        if (widgetStartX < 0 && (widgetStartX % TileLoader::TILE_SIZE)) --tileX0;
        int64_t tileY0 = widgetStartY / TileLoader::TILE_SIZE;
        if (widgetStartY < 0 && (widgetStartY % TileLoader::TILE_SIZE)) --tileY0;
        int64_t tileX1 = (widgetEndX - 1) / TileLoader::TILE_SIZE;
        int64_t tileY1 = (widgetEndY - 1) / TileLoader::TILE_SIZE;

        for (int64_t ty = tileY0; ty <= tileY1; ++ty) {
            for (int64_t tx = tileX0; tx <= tileX1; ++tx) {
                int64_t tileLeft = tx * TileLoader::TILE_SIZE;
                int64_t tileTop = ty * TileLoader::TILE_SIZE;
                int64_t copyStartX = max(tileLeft, (int64_t)widgetStartX);
                int64_t copyStartY = max(tileTop, (int64_t)widgetStartY);
                int copyEndX = (int)min(tileLeft + TileLoader::TILE_SIZE, (int64_t)widgetEndX);
                int copyEndY = (int)min(tileTop + TileLoader::TILE_SIZE, (int64_t)widgetEndY);

                int srcX = (int)(copyStartX - tileLeft);
                int srcY = (int)(copyStartY - tileTop);
                int copyW = copyEndX - (int)copyStartX;
                int copyH = copyEndY - (int)copyStartY;

                int destX = x + (int)(copyStartX - widgetStartX);
                int destY = y + (int)(copyStartY - widgetStartY);

                if (copyW <= 0 || copyH <= 0) continue;

                uint32_t tileUx = (uint32_t)tx;
                uint32_t tileUy = (uint32_t)ty;

                if (TileLoader::tileExists(zoom, tileUx, tileUy)) {
                    if (TileLoader::loadRawTileToCanvas(zoom, tileUx, tileUy, destX, destY, srcX, srcY, copyW, copyH)) {
                        tilesLoaded = true;
                    }
                }
            }
        }
    }

    if (tilesLoaded) {
        // draw polyline on top of tiles
        int px = 0, py = 0;
        bool first = true;
        for (const auto& p : track) {
            int sx, sy;
            project(p.lat, p.lon, sx, sy);
            if (first) { px = sx; py = sy; first = false; }
            else { canvas.drawLine(px, py, sx, sy, ST77XX_GREEN); px = sx; py = sy; }
        }
        const auto& last = track.back();
        int lx, ly; project(last.lat, last.lon, lx, ly);
        canvas.fillCircle(lx, ly, 3, ST77XX_RED);

        Disp::markDirty(x, y, _width, _height);
        _dirty = false;
        return;
    }

    // fallback: render vector polyline
    if (track.empty()) {
        Disp::drawText(x + 4, y + 4, String("No GPS"), ST77XX_WHITE);
        Disp::markDirty(x, y, _width, _height);
        _dirty = false;
        return;
    }

    int px = 0, py = 0;
    bool first = true;
    for (const auto& p : track) {
        int sx, sy;
        project(p.lat, p.lon, sx, sy);
        if (first) { px = sx; py = sy; first = false; }
        else { canvas.drawLine(px, py, sx, sy, ST77XX_GREEN); px = sx; py = sy; }
    }
    const auto& last = track.back();
    int lx, ly; project(last.lat, last.lon, lx, ly);
    canvas.fillCircle(lx, ly, 3, ST77XX_RED);

    Disp::markDirty(x, y, _width, _height);
    _dirty = false;
}

#include "TileLoader.hpp"
#include <cmath>
#include <cstdio>

using namespace TileLoader;

void TileLoader::latLonToTileXY(double lat, double lon, int zoom, uint32_t& outX, uint32_t& outY) {
    // Web Mercator / Slippy tile formula
    double latRad = lat * M_PI / 180.0;
    uint32_t n = 1u << zoom;
    double xtile = (lon + 180.0) / 360.0 * n;
    double ytile = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n;
    outX = (uint32_t)floor(xtile);
    outY = (uint32_t)floor(ytile);
}

void TileLoader::latLonToTilePixel(double lat, double lon, int zoom, int& pixelX, int& pixelY) {
    double latRad = lat * M_PI / 180.0;
    double n = (double)(1u << zoom);
    double xtile = (lon + 180.0) / 360.0 * n;
    double ytile = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n;

    double fracX = xtile - floor(xtile);
    double fracY = ytile - floor(ytile);

    pixelX = (int)floor(fracX * TILE_SIZE);
    pixelY = (int)floor(fracY * TILE_SIZE);
}

static String tilePath(int z, uint32_t x, uint32_t y) {
    // path: /tiles/{z}/{x}/{y}.raw
    char buf[128];
    snprintf(buf, sizeof(buf), "/tiles/%d/%u/%u.raw", z, x, y);
    return String(buf);
}

bool TileLoader::tileExists(int z, uint32_t x, uint32_t y) {
    auto storage = HAL::inst().SD();
    if (!storage) return false;
    String p = tilePath(z, x, y);
    return storage->exists(p.c_str());
}

bool TileLoader::loadRawTileToCanvas(int z, uint32_t x, uint32_t y, int destX, int destY, int srcX, int srcY, int copyW, int copyH) {
    auto storage = HAL::inst().SD();
    if (!storage) return false;

    String p = tilePath(z, x, y);
    if (!storage->exists(p.c_str())) return false;

    File32 f = storage->openFile(p.c_str(), O_RDONLY);
    if (!f) {
        Serial.printf("[TileLoader] failed open %s\n", p.c_str());
        return false;
    }

    // clamp src region
    if (srcX < 0) { copyW += srcX; destX -= srcX; srcX = 0; }
    if (srcY < 0) { copyH += srcY; destY -= srcY; srcY = 0; }
    if (srcX >= TILE_SIZE || srcY >= TILE_SIZE) { f.close(); return false; }
    if (srcX + copyW > TILE_SIZE) copyW = TILE_SIZE - srcX;
    if (srcY + copyH > TILE_SIZE) copyH = TILE_SIZE - srcY;

    // clamp dest region to canvas
    if (destX >= SCREEN_WIDTH || destY >= SCREEN_HEIGHT) { f.close(); return false; }
    if (destX < 0) {
        // adjust srcX to skip left columns
        int skip = -destX;
        srcX += skip;
        copyW -= skip;
        destX = 0;
    }
    if (destY < 0) {
        int skip = -destY;
        srcY += skip;
        copyH -= skip;
        destY = 0;
    }
    if (copyW <= 0 || copyH <= 0) { f.close(); return false; }

    if (destX + copyW > SCREEN_WIDTH) copyW = SCREEN_WIDTH - destX;
    if (destY + copyH > SCREEN_HEIGHT) copyH = SCREEN_HEIGHT - destY;

    // temporary row buffer (max TILE_SIZE pixels)
    static uint16_t rowBuf[TILE_SIZE];

    uint16_t* canvasBuf = canvas.getBuffer();
    canvasBuf += destY * SCREEN_WIDTH + destX;

    // file is raw RGB565 row-major, TILE_SIZE width. Seek to srcY row start.
    for (int row = 0; row < copyH; ++row) {
        uint32_t rowIndex = (uint32_t)(srcY + row);
        uint32_t rowOffset = rowIndex * TILE_SIZE * 2u + (uint32_t)(srcX * 2);
        if (!f.seekSet(rowOffset)) break;

        size_t bytesToRead = copyW * 2;
        if (f.read(rowBuf, bytesToRead) != (int)bytesToRead) {
            break;
        }

        // copy into canvas row (canvas width may be larger than copyW)
        memcpy(canvasBuf, rowBuf, bytesToRead);
        canvasBuf += SCREEN_WIDTH; // next canvas row
    }

    f.close();
    return true;
}
